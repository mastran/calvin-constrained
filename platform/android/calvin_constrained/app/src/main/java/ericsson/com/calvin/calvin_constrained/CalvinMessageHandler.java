package ericsson.com.calvin.calvin_constrained;

/**
 * Created by alexander on 2017-02-13.
 */

public abstract class CalvinMessageHandler {
    public static final String RUNTIME_STOP = "RS";
    public static final String RUNTIME_CALVIN_MSG = "CM";
    public static final String RUNTIME_STARTED = "RR";
    public static final String FCM_CONNECT = "FC";

    public abstract String getCommand();
    public abstract void handleData(byte[] data);
}