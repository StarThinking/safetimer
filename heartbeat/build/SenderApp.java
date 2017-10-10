/* SenderApp.java */

public class SenderApp {
    native int init_sender(); 
    native void destroy_sender(); 
    static {
        System.loadLibrary("sender"); /* (2) */
    }

    static public void main(String argv[]) {
        SenderApp hbSender = new SenderApp();
        hbSender.init_sender(); /* (3) */
        System.out.println("hbSender is created.");
        try {
            Thread.sleep(10000);
        } catch (Exception ex) {
        }
        hbSender.destroy_sender();
        System.out.println("Exit");
    }
}
