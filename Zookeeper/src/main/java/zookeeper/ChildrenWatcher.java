package zookeeper;

import org.apache.log4j.Logger;
import org.apache.zookeeper.KeeperException;
import org.apache.zookeeper.WatchedEvent;
import org.apache.zookeeper.Watcher;
import org.apache.zookeeper.ZooKeeper;
import org.apache.zookeeper.data.Stat;

public class ChildrenWatcher implements Watcher {

    private static Logger log = Logger.getLogger(ChildrenWatcher.class);
    private final ZooKeeper zk;
    private String znode;

    public ChildrenWatcher(ZooKeeper zk, String znode) {
        this.zk = zk;
        this.znode = znode;
    }

    @Override
    public void process(WatchedEvent event) {
        try {
            countChildren();
        } catch (KeeperException | InterruptedException e) {
            log.error(e.getMessage());
        }
    }

    private void countChildren() throws KeeperException, InterruptedException {
        Stat stat = zk.exists(znode, false);
        if (stat != null) {
            zk.getChildren(znode, this);
            int childNumber = stat.getNumChildren();
            System.out.println(String.format("Number of child znodes: %d", childNumber));
        }
    }
}