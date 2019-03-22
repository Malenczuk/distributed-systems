package pl.edu.agh.dsrg.sr;

import org.jgroups.JChannel;
import org.jgroups.Message;
import org.jgroups.protocols.*;
import org.jgroups.protocols.pbcast.*;
import org.jgroups.stack.ProtocolStack;

import java.net.InetAddress;

public class Channel extends JChannel {

    public Channel() throws Exception {
        super(false);
        ProtocolStack stack = new ProtocolStack();
        this.setProtocolStack(stack);
        stack.addProtocol(new UDP().setValue("mcast_group_addr", InetAddress.getByName("230.100.213.7")))
                .addProtocol(new PING())
                .addProtocol(new MERGE3())
                .addProtocol(new FD_SOCK())
                .addProtocol(new FD_ALL()
                        .setValue("timeout", 12000)
                        .setValue("interval", 3000))
                .addProtocol(new VERIFY_SUSPECT())
                .addProtocol(new BARRIER())
                .addProtocol(new NAKACK2())
                .addProtocol(new UNICAST3())
                .addProtocol(new STABLE())
                .addProtocol(new GMS())
                .addProtocol(new UFC())
                .addProtocol(new MFC())
                .addProtocol(new FRAG2())
                .addProtocol(new STATE())
                .addProtocol(new SEQUENCER())
                .addProtocol(new FLUSH());

        stack.init();
    }

    public void init(String cluster, DistributedMap map) throws Exception {
        this.setReceiver(new Receiver(this, map));
        this.connect(cluster);
        this.getState(null, 0);
    }

    public void send(String s) throws Exception {
        Message msg = new Message(null, null, s);
        this.send(msg);
    }

}
