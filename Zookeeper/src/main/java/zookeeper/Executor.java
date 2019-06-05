package zookeeper;

import org.apache.log4j.Logger;
import org.apache.log4j.PropertyConfigurator;
import org.apache.zookeeper.WatchedEvent;
import org.apache.zookeeper.Watcher;
import org.apache.zookeeper.ZooKeeper;
import zookeeper.console.Console;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Properties;

public class Executor implements Watcher, Runnable, DataMonitor.DataMonitorListener {

    private static Logger log = Logger.getLogger(Executor.class);
    private final String hostPort;
    private final String znode;
    private final String[] exec;
    private final ZooKeeper zk;
    private final DataMonitor dm;

    private Process child = null;
    private boolean running = true;

    public Executor(String hostPort, String znode, String[] exec) throws IOException {
        this.hostPort = hostPort;
        this.znode = znode;
        this.exec = exec;

        zk = new ZooKeeper(hostPort, 3000, this);
        dm = new DataMonitor(zk, znode, null, this);
    }

    public static void main(String[] args) {
        if (args.length < 3) {
            log.error("USAGE: Executor hostPort znode program [args ...]");
            System.exit(2);
        }

        configureLogger();

        String hostPort = args[0];
        String znode = args[1];
        String[] exec = new String[args.length - 2];
        System.arraycopy(args, 2, exec, 0, exec.length);

        try {
            new Executor(hostPort, znode, exec).run();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    private static void configureLogger() {
        Properties props = new Properties();
        try (InputStream configStream = Executor.class.getClassLoader().getResourceAsStream("log4j.properties")) {
            props.load(configStream);
        } catch (IOException e) {
            log.error("Error: Cannot load configuration file ");
        }
        props.setProperty("log4j.appender.FILE.file", "zoo.log");
        PropertyConfigurator.configure(props);
    }

    @Override
    public void run() {
        new Thread(new Console(zk, znode, this)).start();

        try {
            synchronized (this) {
                while (!dm.isDead() && running) {
                    wait();
                }
            }
        } catch (InterruptedException e) {
            log.error(e.getMessage());
        }
    }

    public void close() {
        running = false;
        synchronized (this) {
            notifyAll();
        }
    }

    @Override
    public void closing(int rc) {
        synchronized (this) {
            notifyAll();
        }
    }

    @Override
    public void process(WatchedEvent event) {
        dm.process(event);
    }

    @Override
    public void exists(byte[] data) {
        if (data == null) {
            if (child != null) {
                System.out.println("Killing process");
                child.destroyForcibly();
                child.destroy();
                try {
                    child.waitFor();
                } catch (InterruptedException ignored) {
                }
            }
            child = null;
        } else {
            if (child != null) {
                System.out.println("Stopping child");
                child.destroy();
                try {
                    child.waitFor();
                } catch (InterruptedException e) {
                    log.error(e.getMessage());
                }
            }
            try {
                System.out.println("Starting child ");
                child = Runtime.getRuntime().exec(exec);
                new StreamWriter(child.getInputStream(), System.out);
                new StreamWriter(child.getErrorStream(), System.err);
            } catch (IOException e) {
                log.error(e.getMessage());
            }
        }
    }

    static class StreamWriter extends Thread {
        OutputStream os;

        InputStream is;

        StreamWriter(InputStream is, OutputStream os) {
            this.is = is;
            this.os = os;
            start();
        }

        public void run() {
            byte[] b = new byte[80];
            int rc;
            try {
                while ((rc = is.read(b)) > 0) {
                    os.write(b, 0, rc);
                }
            } catch (IOException ignored) {
            }
        }
    }
}
