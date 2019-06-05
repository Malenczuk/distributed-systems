package zookeeper.console;


import org.apache.log4j.Logger;
import org.apache.zookeeper.ZooKeeper;
import zookeeper.Executor;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;


public class Console implements Runnable {

    private static Logger log = Logger.getLogger(Console.class);
    private final Printer printer;
    private final String znode;
    private final Executor executor;

    public Console(ZooKeeper zk, String znode, Executor executor) {
        this.printer = new Printer(zk);
        this.znode = znode;
        this.executor = executor;
    }

    @Override
    public void run() {
        BufferedReader br = new BufferedReader(new InputStreamReader(System.in));
        while (true) {
            try {
                String line = br.readLine().trim();

                switch (line) {
                    case "exit":
                        executor.close();
                        return;
                    case "ls":
                        System.out.println(printer.printTree(znode));
                }
            } catch (IOException e) {
                log.error("Error reading input line");
            }
        }
    }
}
