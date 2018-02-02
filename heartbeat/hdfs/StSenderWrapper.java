package org.apache.hadoop.hdfs.server.datanode;

import java.io.*;
import java.net.*;
import java.nio.*;

class StSenderWrapper {
   private static String stateServer = "127.0.0.1";
   private static int port = 5003;
   
   public static long stSendHeartbeat(long timeout) {
      long ret = 0;

      try {
      DatagramSocket clientSocket = new DatagramSocket();
      DatagramPacket sendPacket, receivePacket;
      InetAddress IPAddress = InetAddress.getByName(stateServer);
      ByteBuffer sendBuffer, receiveBuffer;
      byte[] sendBytes, receiveBytes;
      //long appId = 1;
      boolean success = false;

      while (!success) {
            // Send request
            sendBuffer = ByteBuffer.allocate(Long.SIZE * 2/ Byte.SIZE);
            sendBuffer.order(ByteOrder.LITTLE_ENDIAN);
            // Convert to int
            sendBytes = sendBuffer.putLong(0).putLong(timeout).array();
            sendPacket = new DatagramPacket(sendBytes, sendBytes.length, IPAddress, port);
            clientSocket.send(sendPacket);
            System.out.println("[msx] stSendHeartbeat request sent");

            // Receive response
            receiveBytes = new byte[Long.SIZE / Byte.SIZE];
            receivePacket = new DatagramPacket(receiveBytes, receiveBytes.length);
            clientSocket.setSoTimeout(100);
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
            ret = receiveBuffer.getLong();
            success = true;

            clientSocket.close();
      }
      } catch(Exception e) {
          e.printStackTrace();
      }
      return ret;
   }
   
   public static long stUpdateValidTime(long time) {
      long ret = 0;
        
      try {
      DatagramSocket clientSocket = new DatagramSocket();
      DatagramPacket sendPacket, receivePacket;
      InetAddress IPAddress = InetAddress.getByName(stateServer);
      ByteBuffer sendBuffer, receiveBuffer;
      byte[] sendBytes, receiveBytes;
      //long appId = 1;
      boolean success = false;

      while (!success) {
            // Send request
            sendBuffer = ByteBuffer.allocate(Long.SIZE * 2/ Byte.SIZE);
            sendBuffer.order(ByteOrder.LITTLE_ENDIAN);
            // Convert to int
            sendBytes = sendBuffer.putLong(1).putLong(time).array();
            sendPacket = new DatagramPacket(sendBytes, sendBytes.length, IPAddress, port);
            clientSocket.send(sendPacket);
            System.out.println("[msx] stUpdateValidTime request sent");

            // Receive response
            receiveBytes = new byte[Long.SIZE / Byte.SIZE];
            receivePacket = new DatagramPacket(receiveBytes, receiveBytes.length);
            clientSocket.setSoTimeout(100);
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
            ret = receiveBuffer.getLong();
            success = true;

            clientSocket.close();
      }
      } catch(Exception e) {
          e.printStackTrace();
      }
      return ret;
   }
}
