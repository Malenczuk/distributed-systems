package zookeeper.console;

import org.apache.log4j.Logger;
import org.apache.zookeeper.KeeperException;
import org.apache.zookeeper.ZooKeeper;
import org.apache.zookeeper.data.Stat;

public class Printer {

    private static Logger log = Logger.getLogger(Printer.class);
    private final ZooKeeper zk;

    Printer(ZooKeeper zk) {
        this.zk = zk;
    }

    public String printTree(String node) {
        Stat stat = null;
        StringBuilder sb = new StringBuilder();
        try {
            stat = zk.exists(node, false);
        } catch (KeeperException | InterruptedException e) {
            log.error(e.getMessage());
        }
        if (stat != null) printTree(node, 0, sb);

        return sb.toString();
    }

    private void printTree(String node, int indent, StringBuilder sb) {
        printNode(node, indent, sb);
        try {
            zk.getChildren(node, false).forEach(child ->
                    printTree(node.concat("/" + child), indent + 1, sb));
        } catch (KeeperException | InterruptedException e) {
            log.error(e.getMessage());
        }
    }

    private void printNode(String node, int indent, StringBuilder sb) {
        String name = node.substring(node.lastIndexOf("/"));

        sb.append(getIndentString(indent));
        sb.append("├──");
        sb.append(name);
        sb.append("\n");
    }

    private String getIndentString(int indent) {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < indent; i++) {
            sb.append("│  ");
        }
        return sb.toString();
    }

}
