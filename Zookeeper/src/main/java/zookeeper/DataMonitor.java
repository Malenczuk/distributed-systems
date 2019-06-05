package zookeeper;

import org.apache.log4j.Logger;
import org.apache.zookeeper.*;
import org.apache.zookeeper.data.Stat;

import java.util.Arrays;

import static org.apache.zookeeper.KeeperException.Code.SESSIONEXPIRED;
import static org.apache.zookeeper.KeeperException.Code.get;

public class DataMonitor implements Watcher, AsyncCallback.StatCallback {

    private static Logger log = Logger.getLogger(DataMonitor.class);
    private final ZooKeeper zk;
    private final String znode;
    private final Watcher chainedWatcher;
    private final DataMonitorListener listener;

    private byte[] prevData;
    private boolean dead = false;

    public DataMonitor(ZooKeeper zk, String znode, Watcher chainedWatcher, DataMonitorListener listener) {
        this.zk = zk;
        this.znode = znode;
        this.chainedWatcher = chainedWatcher;
        this.listener = listener;

        zk.exists(znode, true, this, null);
    }

    @Override
    public void processResult(int rc, String path, Object ctx, Stat stat) {
        boolean exists;
        switch (get(rc)) {
            case OK:
                exists = true;
                break;
            case NONODE:
                exists = false;
                break;
            case SESSIONEXPIRED:
            case NOAUTH:
                dead = true;
                listener.closing(rc);
                return;
            default:
                zk.exists(znode, true, this, null);
                return;
        }

        byte[] b = null;
        if (exists) {
            try {
                b = zk.getData(znode, false, null);
            } catch (KeeperException e) {
                log.error(e.getMessage());
            } catch (InterruptedException e) {
                return;
            }
        }

        if ((b == null && null != prevData) || (b != null && !Arrays.equals(prevData, b))) {
            listener.exists(b);
            prevData = b;
        }
    }

    @Override
    public void process(WatchedEvent event) {
        String path = event.getPath();

        if (event.getType() == Event.EventType.None) {
            switch (event.getState()) {
                case SyncConnected:
                    try {
                        Stat stat = zk.exists(znode, false);
                        if (stat != null) watchChildren(znode);
                    } catch (KeeperException | InterruptedException e) {
                        log.error(e.getMessage());
                    }
                    break;
                case Expired:
                    dead = true;
                    listener.closing(SESSIONEXPIRED.intValue());
                    break;
            }
        } else if (path != null && path.equals(znode)) {
            zk.exists(znode, true, this, null);

            try {
                if (event.getType() == Event.EventType.NodeCreated) watchChildren(znode);
            } catch (KeeperException | InterruptedException e) {
                log.error(e.getMessage());
            }
        }

        if (chainedWatcher != null) {
            chainedWatcher.process(event);
        }
    }


    public boolean isDead() {
        return dead;
    }

    private void watchChildren(String node) throws KeeperException, InterruptedException {
        zk.getChildren(node, new ChildrenWatcher(zk, node));
    }

    interface DataMonitorListener {
        void exists(byte[] data);

        void closing(int rc);
    }
}
