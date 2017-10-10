/* HelloWorld.java */

public class DataNode {

    static {
            System.load("/root/hb-latency/call_from_java/libctest.so");
    }

//    static class HelloWorld {
        native static void helloFromC();
  //  }

    static public void main(String argv[]) {
        helloFromC();
        //HelloWorld helloWorld = new HelloWorld();
        //helloWorld.helloFromC(); /* (3) */
    }
}
