import java.io.*;
import java.net.*;
import java.nio.*;

class NodeStateClient {
   private static String stateServer = "10.0.0.11";
   private static int port = 5003;
   
   public static Long getNodeState(String nodeIp) throws Exception {
      Long state = new Long(0);
      DatagramSocket clientSocket = new DatagramSocket();
      DatagramPacket sendPacket, receivePacket;
      InetAddress IPAddress = InetAddress.getByName(stateServer);
      ByteBuffer sendBuffer, receiveBuffer;
      byte[] sendBytes, receiveBytes;
      long appId = 1;
      boolean success = false;

      while (!success) {
      // Send state request
      sendBuffer = ByteBuffer.allocate(Long.SIZE * 2/ Byte.SIZE);
      sendBuffer.order(ByteOrder.LITTLE_ENDIAN);
      InetAddress addr = InetAddress.getByName(nodeIp);
      long nodeIpLong = (long)ByteBuffer.wrap(addr.getAddress()).getInt();
      System.out.println("nodeIpLong is " + nodeIpLong + " " + ByteBuffer.wrap(addr.getAddress()).getInt());
      // Convert to int
      sendBytes = sendBuffer.putLong(appId).putLong(nodeIpLong).array();
      sendPacket = new DatagramPacket(sendBytes, sendBytes.length, IPAddress, port);
      clientSocket.send(sendPacket);
      System.out.println("State request sent");

      //try{
      //  Thread.sleep(1000);
      //} catch(Exception e) {
      //}

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
      state = receiveBuffer.getLong();
      System.out.println("State is " + state);
      success = true;

      clientSocket.close();
      }

      return state;
   }

   public static void main(String[] args) throws Exception {
      System.out.println("10.0.0.11: ");
      NodeStateClient.getNodeState("10.0.0.11");
      
      System.out.println("10.0.0.12: ");
      NodeStateClient.getNodeState("10.0.0.12");
   }
}
