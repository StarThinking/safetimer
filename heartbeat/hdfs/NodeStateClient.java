import java.io.*;
import java.net.*;
import java.nio.*;

class NodeStateClient {
   private static String stateServer = "10.0.0.11";
   private static int port = 5003;
   private static Long appId = new Long(1);
   
   public static Long getNodeState() throws Exception {
      Long state;
      DatagramSocket clientSocket = new DatagramSocket();
      DatagramPacket sendPacket, receivePacket;
      InetAddress IPAddress = InetAddress.getByName(stateServer);
      ByteBuffer sendBuffer, receiveBuffer;
      byte[] sendBytes, receiveBytes;

      // Send state request
      sendBuffer = ByteBuffer.allocate(Long.SIZE / Byte.SIZE);
      sendBuffer.order(ByteOrder.LITTLE_ENDIAN);
      sendBytes = sendBuffer.putLong(appId).array();
      sendPacket = new DatagramPacket(sendBytes, sendBytes.length, IPAddress, port);
      clientSocket.send(sendPacket);
      System.out.println("State request sent");

      // Receiver state response
      receiveBytes = new byte[Long.SIZE / Byte.SIZE];
      receivePacket = new DatagramPacket(receiveBytes, receiveBytes.length);
      clientSocket.receive(receivePacket);
      receiveBuffer = ByteBuffer.allocate(Long.SIZE / Byte.SIZE);
      receiveBuffer.put(receiveBytes);
      receiveBuffer.order(ByteOrder.LITTLE_ENDIAN);
      receiveBuffer.flip();
      state = receiveBuffer.getLong();
      System.out.println("State is " + state);

      clientSocket.close();
      return state;
   }

   //public static void main(String[] args) throws Exception {
   //   NodeStateClient.sendNodeStateRequest();
   //}
}
