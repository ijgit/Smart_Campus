import sys
from PyQt5.QtWidgets import QApplication, QWidget, QLabel, QTextEdit, QVBoxLayout, QPushButton, QDesktopWidget
import requests
import datetime as dt
import numpy as np
import sys
import pandas as pd


class MyApp(QWidget):

    def __init__(self):
        super().__init__()
        self.initUI()

    def initUI(self):
        self.lbl1 = QLabel('Created Time After (CRA)')

        self.cra = QTextEdit()
        self.cra.setText('20200528')
        self.cra.setAcceptRichText(False)

        self.lbl2 = QLabel('Created Time Before (CRB)')
        self.crb = QTextEdit()
        self.crb.setText('20200528')
        self.crb.setAcceptRichText(False)

        self.btn = QPushButton('GET', self)
        self.btn.clicked.connect(self.get_data)

        vbox = QVBoxLayout()
        vbox.addWidget(self.lbl1)
        vbox.addWidget(self.cra)
        vbox.addWidget(self.lbl2)
        vbox.addWidget(self.crb)
        vbox.addWidget(self.btn)
        vbox.addStretch()

        self.setLayout(vbox)
        self.setWindowTitle('Get Data')
        self.setGeometry(300, 300, 300, 200)
        self.center()
        self.show()

    def get_data(self):
        cra = self.cra.toPlainText()
        crb = self.crb.toPlainText()
        cse = "http://203.253.128.161:7579/Mobius/"
        ae = 'ubicomp_lora/'
        time_dif = 'T143000'
        cra_ = str(int(cra) - 1)
        query = f'?rcn=4&cra={cra_ + time_dif}&crb={crb + time_dif}'
        payload = {}
        headers = {
            'Accept': 'application/json',
            'X-M2M-RI': '12345',
            'X-M2M-Origin': 'SOrigin'
        }

        cra_d = dt.datetime.strptime(cra[0:8], '%Y%m%d')
        crb_d = dt.datetime.strptime(crb[0:8], '%Y%m%d') + dt.timedelta(days=1)
        d_dif = crb_d - cra_d
        dif_min = d_dif.total_seconds() // 60
        DATA_LEN = int(dif_min // 30)

        node_num = 15
        data = pd.DataFrame(np.nan, index=range(DATA_LEN * node_num), columns=range(13))

        base = 0

        for i in range(1, 16):
            k = i if (i != 4) else 16

            cnt = str(k)
            url = cse + ae + cnt + query
            r = requests.get(url, headers=headers, data=payload)

            if r.status_code != 200:
                sys.exit(0)

            cin = r.json()['m2m:rsp']['m2m:cin']
            cin = pd.DataFrame(cin)['con']
            cin_ = pd.DataFrame(cin.str.replace('\r', '').str.split(',').tolist())

            d_str = cin_[3] + ' ' + cin_[4]
            d_format = '%Y/%m/%d %H:%M:%S'
            for i in range(0, len(cin_)):
                crd_d = dt.datetime.strptime(d_str[i], d_format)
                cin_.loc[i, 13] = int(((crd_d - cra_d).total_seconds() // 60) // 30)

            cin_ = cin_.astype({13: 'int'})
            cin_.sort_values(by=13, inplace=True)  # 시간 순서 정렬
            cin = cin_.to_numpy()  # numpy 로 안바꿔서 넣으면 행 순서가 뒤죽박죽 됨

            for i in range(len(cin)):
                data.loc[base + cin[i, 13]] = cin[i, :13]

            data[0][base:base + DATA_LEN].fillna(k, inplace=True)

            base = base + DATA_LEN

        data.columns = ['noid', 'ix_tot', 'ix_day', 'yymmdd', 'hhmmss', 'lati', 'long', 'alti', 'pm1', 'pm2_5', 'pm10',
                        'temp', 'batt']
        filename = f'{cra}_{crb}.csv'
        data.to_csv(filename, na_rep='nan', index=False)

    def center(self):
        qr = self.frameGeometry()
        cp = QDesktopWidget().availableGeometry().center()
        qr.moveCenter(cp)
        self.move(qr.topLeft())


if __name__ == '__main__':
    app = QApplication(sys.argv)
    ex = MyApp()
    sys.exit(app.exec_())
