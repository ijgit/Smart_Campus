import java.net.*;
import java.io.*;
import java.util.Base64;
import java.util.Base64.Decoder;



public class kerlink_udp_server {
	
	public static void main(String[] args) throws IOException, InterruptedException{
		DatagramSocket sSock;
		DatagramPacket sPkt, rPkt;
		InetAddress clientIp;
		int cPort, sPort = 1234;
		
		
		try {
			sSock = new DatagramSocket(sPort);
			System.out.println("Server is running.");
			
			while(true) {
				byte[] buffer = new byte[1024];
				
				rPkt = new DatagramPacket(buffer, buffer.length);
				sSock.receive(rPkt);
				
				String strIn = new String(rPkt.getData(), 0, rPkt.getLength());
				clientIp = rPkt.getAddress();
				cPort = rPkt.getPort();
				
				System.out.println("[Client" + clientIp + ":" + cPort + "] " + strIn);
				
				
				//parsing ,			
				String str = strIn;
				String[] array = str.split("\"");
				//for(int i=0; i<array.length; i++) {
				//	System.out.println(i + ": " + array[i]);
				//}
				
				String moteeui = array[5];
				String payload = array[19];
				String port = array[16];
			//	for(int i=0; i<port.length(); i++) {
			//		System.out.println(i + ": " + port.charAt(i));
			//	}
								
				if(port.charAt(2) == '2') {
					// base64 decoding
					Decoder decoder = Base64.getDecoder();
					byte[] decoded_payload_arr = decoder.decode(payload);
					for(int i =0; i<decoded_payload_arr.length; i++) {
						if(decoded_payload_arr[i] == 'c')
							decoded_payload_arr[i] = ',';
						else if(decoded_payload_arr[i] == 'd') decoded_payload_arr[i] = '.';
					}
					String decoded_payload = new String(decoded_payload_arr, "UTF-8");
					String dev_id = decoded_payload.split(",")[0];
					
					
					System.out.println("moteeui: " + moteeui);
					System.out.println("payload: " + payload);
					System.out.println("decoded payload: " + decoded_payload);
					
					// process node program (post to mobius server);
					Runtime rt = Runtime.getRuntime();
					Process pc = null;
					try {
						pc = rt.exec("node post_request.js " + dev_id + " " + decoded_payload);
						System.out.println("post request sended");
					}catch(IOException e) {
						e.printStackTrace();
					}finally {
						pc.waitFor();
						pc.destroy();
					}		
				}else {
					;
					//System.out.println("dummy port: " + port );
					//System.out.println("dummy payload: " + payload );
				}
				
				if(strIn.trim().equals("quit")) break;
			}
			sSock.close();
			System.out.println("UDP server is Closed.");
		}catch(Exception e) {
			System.out.println(e);
		}
	}
}
