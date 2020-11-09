import sys
import requests
import datetime as dt
import numpy as np
import sys
import pandas as pd

create_after = '20200601'
create_before = '20200603'


def get_data():
        cra = create_after
        crb = create_before

        cse = "http://203.253.128.161:7579/Mobius/"
        ae = 'ubicomp_lora/'
        time_dif = 'T143000'
        cra_ = str(int(cra) - 1)
        query = f"?rcn=4&cra={cra_ + time_dif}&crb={crb + time_dif}"
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


get_data();