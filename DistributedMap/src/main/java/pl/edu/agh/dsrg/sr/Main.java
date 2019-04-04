package pl.edu.agh.dsrg.sr;

import java.io.BufferedReader;
import java.io.InputStreamReader;

public class Main {
    public static void main(String[] args) throws Exception {
        System.setProperty("java.net.preferIPv4Stack","true");
        DistributedMap map = new DistributedMap("cluster");
        InputStreamReader inp = new InputStreamReader(System.in);
        BufferedReader reader = new BufferedReader(inp);
        String msg;

        for(boolean c = true; c; c = !msg.equals("quit")){
            msg = reader.readLine();
            System.out.println(execute(map, msg));
        }
        reader.close();
    }

    public static Object execute(DistributedMap map, String msg) {
        var cmd = DistributedMap.parseCommand(msg);
        if (cmd.getVal1() != null) {
            switch (cmd.getVal1()) {
                case "put":
                    if (cmd.getVal3() != null)
                        return map.put(cmd.getVal2(), cmd.getVal3());
                    break;
                case "get":
                    return map.get(cmd.getVal2());
                case "remove":
                    return map.remove(cmd.getVal2());
                case "containsKey":
                    return map.containsKey(cmd.getVal2());
            }
        }
        return "Wrong Command";
    }
}
