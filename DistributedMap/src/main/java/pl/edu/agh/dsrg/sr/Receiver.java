package pl.edu.agh.dsrg.sr;

import org.jgroups.*;
import org.jgroups.util.Util;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.HashMap;
import java.util.Map;


public class Receiver extends ReceiverAdapter {
    private final DistributedMap state;
    private final JChannel channel;

    Receiver(JChannel channel, DistributedMap map) {
        this.channel = channel;
        this.state = map;
    }

    @Override
    public void receive(Message msg) {
        String message = (String) msg.getObject();
        System.out.println("Received message from " + msg.getSrc() + " - " + message);

        var cmd = DistributedMap.parseCommand(message);
        switch (cmd.getVal1()){
            case "put":
                state.getLocalCopy().put(cmd.getVal2(), cmd.getVal3());
                break;
            case "remove":
                state.getLocalCopy().remove(cmd.getVal2());
        }
    }

    @Override
    public void viewAccepted(View view) {
        System.out.println("received view " + view);
        if (view instanceof MergeView) {
            System.out.println("Merge");
            ViewHandler handler = new ViewHandler(channel, (MergeView) view);
            handler.start();
        }
    }

    @Override
    public void getState(OutputStream output) throws Exception {
        synchronized (state) {
            Util.objectToStream(state.getLocalCopy(), new DataOutputStream(output));
        }
    }

    @Override
    public void setState(InputStream input) throws Exception {
        Map<String, Integer> map;
        map = (HashMap<String, Integer>) Util.objectFromStream(new DataInputStream(input));
        synchronized (state) {
            state.setState(map);
        }
    }

    private static class ViewHandler extends Thread {
        JChannel ch;
        MergeView view;

        private ViewHandler(JChannel ch, MergeView view) {
            this.ch = ch;
            this.view = view;
        }

        public void run() {
            View tmp_view = view.getSubgroups().get(0);
            Address local_addr = ch.getAddress();
            if (!tmp_view.getMembers().contains(local_addr)) {
                System.out.println("Not member of the new primary partition ("
                        + tmp_view + "), will re-acquire the state");
                try {
                    ch.getState(null, 30000);
                } catch (Exception ignored) {
                }
            } else {
                System.out.println("Not member of the new primary partition ("
                        + tmp_view + "), will do nothing");
            }
        }
    }
}
