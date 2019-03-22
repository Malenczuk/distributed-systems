package pl.edu.agh.dsrg.sr;

import org.jgroups.util.Triple;

import java.util.HashMap;
import java.util.Map;

public class DistributedMap implements SimpleStringMap {
    private HashMap<String, Integer> localCopy = new HashMap<>();
    private Channel channel;

    public DistributedMap(String cluster) throws Exception {
        this.channel = new Channel();
        channel.init(cluster, this);
    }

    public void setState(Map<? extends String, ? extends Integer> newState) {
        localCopy.clear();
        localCopy.putAll(newState);
    }

    public HashMap<String, Integer> getLocalCopy() {
        return localCopy;
    }

    @Override
    public boolean containsKey(String key) {
        return localCopy.containsKey(key);
    }

    @Override
    public Integer get(String key) {
        return localCopy.get(key);
    }

    @Override
    public Integer put(String key, Integer value) {
        try {
            channel.send(String.format("put %s %d", key, value));
        } catch (Exception e) {
            e.printStackTrace();
        }
        return localCopy.put(key, value);
    }

    @Override
    public Integer remove(String key) {
        try {
            channel.send(String.format("remove %s", key));
        } catch (Exception e) {
            e.printStackTrace();
        }
        return localCopy.remove(key);
    }

    static public Triple<String, String, Integer> parseCommand(String msg) {
        String cmd = null, key = null;
        Integer value = null;

        String[] split = msg.split("\\s+");

        if (split.length >= 2) {
            cmd = split[0];
            key = split[1];
        }
        if (split.length >= 3) value = Integer.parseInt(split[2]);

        return new Triple<>(cmd, key, value);
    }
}

