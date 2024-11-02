struct LinkedNode {
    double value;
    shared[LinkedNode] next;
    shared[LinkedNode] prev;
    LinkedNode(var value) {
        self.value = value;
    }
};

fn set_next(shared[LinkedNode] from, shared[LinkedNode] to) {
    from.next = to;
    to.prev = from;
}

fn main() {
    var node1 = shared[LinkedNode](1);
    var node2 = shared[LinkedNode](2);
    var node3 = shared[LinkedNode](3);

    set_next(node1, node2);
    set_next(node2, node3);
    try {
        print(node1.value);
        print(node1.next.value);
        print(node1.next.next.value);
        print(node1.next.next.next.value);
    }
    catch(std@runtime_error) {
        print("runtime error");
    }

    return 0;
}