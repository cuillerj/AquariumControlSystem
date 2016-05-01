
import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketTimeoutException;
import java.sql.Connection;
import java.sql.DriverManager;
import java.sql.SQLException;
import java.sql.Statement;
import java.text.DateFormat;
import java.util.Locale;

public class traceDataReceived {
	public static void main(String args[]) throws Exception
	{  

		System.out.println("traceDataReceived");
		int port = Integer.parseInt(args[0]); // port to listen to as argument		
		DatagramSocket serverSocket = new DatagramSocket(port); 
				Locale locale=Locale.FRENCH;
//				int Indst_status;
				String inputMessArray[]={"response/gpio/","information/alive","status/","mesurment/"};
	        	 System.out.println("number of known commands:"+inputMessArray.length);
		         while(true)
		         {
					 java.sql.PreparedStatement preparedStatement = null;
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
		                System.out.println(message+" len:"+message.length());
		                String IDs=message.substring(0, 4);
		                System.out.println(IDs);     
			            int messType=-1;
		                for (int i=0;i<inputMessArray.length;i++)
		                {
		                	if (message.indexOf(inputMessArray[i])!=-1)
		                	{
		                		messType=i;
		                	}
		                }	                
		    			switch (messType)
		    			{
		    			case 0:
		    				System.out.println("case 0 ");
		    			 break;
		    			case 1:
		    				System.out.println("case 1 ");
			    			 break;
		    			case 2:
		    				System.out.println("case 2 ");
			 				 break;
		    			case 3:
		    				System.out.println("case 3 ");
			 					boolean endInd=false;
			 					String IndValue="";
			 					int first=0;
			 					int posInd=message.indexOf("ind_id/");
			 					int startRech=posInd;
			 					int posNextInd=message.indexOf("ind_id/",posInd+1);
			 					int posValue=0;
			 					String IndNb="";
			 					while (endInd==false)
			 					{
			 						
			 					System.out.println(posInd);
			 					System.out.println(startRech);
			 					System.out.println(posNextInd);
					        	
			 					posValue=message.indexOf("=",posInd);

			 					System.out.println(posValue);
				        		}
								if (startRech==-1 )
								{
									endInd=true;
									int posEnd=message.indexOf("\\End");
				 					IndValue=message.substring(posValue+1,posEnd);
				 					IndNb=message.substring(posInd+7,posValue);
								}
								else {
									if (first==0 && posNextInd==-1)
									{
										int posEnd=message.indexOf("\\End");
					 					IndValue=message.substring(posValue+1,posEnd);
					 					IndNb=message.substring(posInd+7,posValue);
					 					endInd=true;
									}
									else
									{
																
									IndValue=message.substring(posValue+1,posNextInd-1);
				 					IndNb=message.substring(posInd+7,posValue);
									first=1;
									}
								}

			 					int k=message.indexOf("ind_id/",posNextInd+1);

			 					posInd=posNextInd;
			 					posNextInd=k;
								startRech=k;					
							
			    			 break;
		           
			             }
		    			}
		    			
			 				catch(SocketTimeoutException s)
		             {
		                System.out.println("Socket timed out!");
		                serverSocket.close();
		                break;
		             }
			 				catch(IOException e)
		             {
		                e.printStackTrace();
		                serverSocket.close();
		                break;
		             }
			 			finally {}
		    			}         
   }
}