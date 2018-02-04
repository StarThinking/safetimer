import java.io.*;
import java.net.*;
import java.nio.*;

class StReceiverWrapper {
   private static String stateServer = "10.10.1.4";
   private static int port = 5003;
   
   public static long st_timestamp_check(String nodeIp) throws Exception {
      long st_timestamp = 0;
      DatagramSocket clientSocket = new DatagramSocket();
      DatagramPacket sendPacket, receivePacket;
      InetAddress IPAddress = InetAddress.getByName(stateServer);
      ByteBuffer sendBuffer, receiveBuffer;
      byte[] sendBytes, receiveBytes;
      long appId = 1;
      boolean success = false;

      while (!success) {
            // Send st_timestamp request
            sendBuffer = ByteBuffer.allocate(Long.SIZE * 2/ Byte.SIZE);
            sendBuffer.order(ByteOrder.LITTLE_ENDIAN);
            InetAddress addr = InetAddress.getByName(nodeIp);
            long nodeIpLong = (long)ByteBuffer.wrap(addr.getAddress()).getInt();
            //System.out.println("nodeIpLong is " + nodeIpLong + " " + ByteBuffer.wrap(addr.getAddress()).getInt());
            // Convert to int
            sendBytes = sendBuffer.putLong(appId).putLong(nodeIpLong).array();
            sendPacket = new DatagramPacket(sendBytes, sendBytes.length, IPAddress, port);
            clientSocket.send(sendPacket);
            //System.out.println("State request sent");

            // Receiver state response
            receiveBytes = new byte[Long.SIZE / Byte.SIZE];
            receivePacket = new DatagramPacket(receiveBytes, receiveBytes.length);
            clientSocket.setSoTimeout(1000);
            try {
                clientSocket.receive(receivePacket);
            } catch(Exception e) { 
                e.printStackTrace();
                continue;
            } 
            receiveBuffer = ByteBuffer.allocate(Long.SIZE / Byte.SIZE);
            receiveBuffer.put(receiveBytes);
            receiveBuffer.order(ByteOrder.LITTLE_ENDIAN);
            receiveBuffer.flip();
            st_timestamp = receiveBuffer.getLong();
            System.out.println("st_timestamp is " + st_timestamp);
            success = true;

            clientSocket.close();
      }

      return st_timestamp;
   }

   public static void main(String[] args) throws Exception {
      System.out.println("10.10.1.1: ");
      StReceiverWrapper.st_timestamp_check("10.10.1.1");
      
      System.out.println("10.10.1.2: ");
      StReceiverWrapper.st_timestamp_check("10.10.1.2");
      
      System.out.println("10.10.1.3: ");
      StReceiverWrapper.st_timestamp_check("10.10.1.3");
      //System.out.println("10.0.0.12: ");
      //NodeStateClient.getNodeState("10.0.0.12");
   }
}
