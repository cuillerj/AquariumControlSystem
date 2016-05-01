import java.io.BufferedReader;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.net.UnknownHostException;
import java.text.DateFormat;
import java.util.Locale;
import java.util.Timer;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.net.*;
import java.sql.*;
// 1.4 modif readbuffer vs readline
class UdpEsp8266ServerTime {
	public static void main(String args[]) throws Exception
	{  
		System.out.println("UdpEsp8266ServerTime V1.0");
		int debug =1;
		DatagramSocket serverSocket = new DatagramSocket(1903); // 
		if (debug==1)
		{
			System.out.println("debug On");
		}
				Locale locale=Locale.FRENCH;
				String inputMessArray[]={"request/time"};
	        	 System.out.println("number of supported commands:"+inputMessArray.length);
		         while(true)
		         {
			 			byte[] receiveData = new byte[1024];
		             try
		             {
			     			DatagramPacket receivePacket = new DatagramPacket(receiveData, receiveData.length);
		                System.out.println("Waiting for client on port " +
		                serverSocket.getLocalPort() + "...");
		    			serverSocket.receive(receivePacket);
		                String message = new String( receivePacket.getData());
		    			InetAddress IPSource = receivePacket.getAddress();

		    			java.util.Date today = new java.util.Date();
		    			DateFormat dateFormat = DateFormat.getDateInstance(DateFormat.SHORT, locale);
		    			DateFormat fullDf = DateFormat.getTimeInstance(DateFormat.LONG,locale);
		                System.out.println("Just connected to "+ IPSource + " " + today);              
	
		        		if (debug==1)
		        		{
		        			System.out.println("BufferedReader");
		        		}
	                System.out.println(message+" len:"+message.length());          
		                String IDs=message.substring(0, 4);
		        		if (debug==1)
		        		{
			                System.out.println(IDs);
		        		}

			            int messType=-1;
		                for (int i=0;i<inputMessArray.length;i++)
		                {
		                	if (message.indexOf(inputMessArray[i])!=-1)
		                	{
		                		messType=i;
		                	}
		                }
		    			String DateChar=dateFormat.format(today);
		    			String TimeChar=fullDf.format(today);
		    			String sendData="respTime "+DateChar+" "+TimeChar+"\\End";
		    			byte[] sendByte;
		    			sendByte=sendData.getBytes();
		    			DatagramSocket clientSocket = new DatagramSocket();
		    			DatagramPacket sendPacket2 = new DatagramPacket(sendByte, sendData.length(), IPSource, 8888);
		    			clientSocket.send(sendPacket2);
		    			clientSocket.close();
		             }

		    			 catch (Exception e) {
								e.printStackTrace();
							} finally {

							}
		         }
	}
}	
