#include <poincare/tree_reference.h>

namespace Poincare {

TreeReference TreeReference::treeClone() const {
  if (!isDefined()){
    return TreeReference(nullptr);
  }
  TreeNode * myNode = node();
  if (myNode->isAllocationFailure()) {
    int allocationFailureNodeId = myNode->allocationFailureNodeIdentifier();
    return TreeReference(TreePool::sharedPool()->node(allocationFailureNodeId));
  }
  TreeNode * nodeCopy = TreePool::sharedPool()->deepCopy(myNode);
  nodeCopy->deepResetReferenceCounter();
  return TreeReference(nodeCopy);
}

TreeReference::~TreeReference() {
  if (isDefined()) {
    assert(node());
    assert(node()->identifier() == m_identifier);
    node()->release();
  }
}

// Hierarchy operations

void TreeReference::addChildTreeAtIndex(TreeReference t, int index, int currentNumberOfChildren) {
  assert(isDefined());
  if (node()->isAllocationFailure()) {
    return;
  }
  if (t.isAllocationFailure()) {
    replaceWithAllocationFailure(currentNumberOfChildren);
    return;
  }
  assert(index >= 0 && index <= currentNumberOfChildren);

  // Retain t before detaching it, else it might get destroyed
  t.node()->retain();

  // Detach t from its parent
  TreeReference tParent = t.parent();
  if (tParent.isDefined()) {
    tParent.removeTreeChild(t);
  }

  // Move t
  TreeNode * newChildPosition = node()->next();
  for (int i = 0; i < index; i++) {
    newChildPosition = newChildPosition->nextSibling();
  }
  TreePool::sharedPool()->move(newChildPosition, t.node(), t.numberOfChildren());
  node()->incrementNumberOfChildren();
}

void TreeReference::removeTreeChildAtIndex(int i) {
  assert(isDefined());
  assert(i >= 0 && i < numberOfChildren());
  TreeReference t = treeChildAtIndex(i);
  removeTreeChild(t);
}

void TreeReference::removeTreeChild(TreeReference t) {
  assert(isDefined());
  TreePool::sharedPool()->move(TreePool::sharedPool()->last(), t.node(), t.numberOfChildren());
  t.node()->release();
  node()->decrementNumberOfChildren();
}

void TreeReference::removeChildren() {
  assert(isDefined());
  node()->releaseChildren(numberOfChildren());
  node()->eraseNumberOfChildren();
}

void TreeReference::replaceWith(TreeReference t) {
  assert(isDefined());
  TreeReference p = parent();
  if (p.isDefined()) {
    p.replaceTreeChildAtIndex(p.node()->indexOfChildByIdentifier(identifier()), t);
  }
}

void TreeReference::replaceTreeChildAtIndex(int oldChildIndex, TreeReference newChild) {
  assert(isDefined());
  if (newChild.isAllocationFailure()) {
    replaceWithAllocationFailure(numberOfChildren());
    return;
  }
  TreeReference p = newChild.parent();
  if (p.isDefined()) {
    p.decrementNumberOfChildren();
  }
  assert(oldChildIndex >= 0 && oldChildIndex < numberOfChildren());
  TreeReference oldChild = treeChildAtIndex(oldChildIndex);
  TreePool::sharedPool()->move(oldChild.node()->nextSibling(), newChild.node(), newChild.numberOfChildren());
  if (!p.isDefined()) {
    newChild.node()->retain();
  }
  TreePool::sharedPool()->move(TreePool::sharedPool()->last(), oldChild.node(), oldChild.numberOfChildren());
  oldChild.node()->release();
}

void TreeReference::replaceWithAllocationFailure(int currentNumberOfChildren) {
  if (isAllocationFailure()) {
    return;
  }
  assert(isDefined());
  TreeReference p = parent();
  bool hasParent = p.isDefined();
  int indexInParentNode = hasParent ? node()->indexInParent() : -1;
  int currentRetainCount = node()->retainCount();
  TreeNode * staticAllocFailNode = node()->failedAllocationStaticNode(); //TODO was typedNode

  // Release all children and delete the node in the pool
  node()->releaseChildrenAndDestroy(currentNumberOfChildren);
  /* WARNING: If we called "p.decrementNumberOfChildren()" here, the number of
   * children of the parent layout would be:
   * -> numberOfChildren() for "dynamic trees" that have a m_numberOfChildren
   * variable (such as HorizontalLayout)
   * -> numberOfChildren() - 1 for "static trees" that have a fixed number of
   * children (such as IntegralLayout)
   *
   * By not decrementing the parent's number of children here, we now that it
   * has (numberOfChildren() - 1) children. */

  /* Create an allocation failure node with the previous node id. We know
   * there is room in the pool as we deleted the previous node and an
   * AllocationFailure nodes size is smaller or equal to any other node size.*/
  //TODO static assert that the size is smaller
  TreeNode * newAllocationFailureNode = TreePool::sharedPool()->deepCopy(staticAllocFailNode);
  newAllocationFailureNode->rename(m_identifier, false);
  if (p.isDefined()) {
    assert(indexInParentNode >= 0);
    /* Set the refCount to previousRefCount-1 because the previous parent is
     * no longer retaining the node. When we add this node to the parent, it
     * will retain it and increment the retain count. */
    newAllocationFailureNode->setReferenceCounter(currentRetainCount - 1);
    p.addChildTreeAtIndex(TreeRef(newAllocationFailureNode), indexInParentNode, p.numberOfChildren() - 1);
  } else {
    newAllocationFailureNode->setReferenceCounter(currentRetainCount);
  }
  p.decrementNumberOfChildren();
  /* We decrement here the parent's number of children, as we did not do it
   * before, see WARNING. */
}

void TreeReference::mergeTreeChildrenAtIndex(TreeReference t, int i) {
  assert(i >= 0 && i <= numberOfChildren());
  // Steal operands
  int numberOfNewChildren = t.numberOfChildren();
  if (i < numberOfChildren()) {
    TreePool::sharedPool()->moveChildren(node()->childAtIndex(i), t.node());
  } else {
    TreePool::sharedPool()->moveChildren(node()->lastDescendant()->next(), t.node());
  }
  t.node()->eraseNumberOfChildren();
  // If t is a child, remove it
  if (node()->hasChild(t.node())) {
    removeTreeChild(t);
  }
  node()->incrementNumberOfChildren(numberOfNewChildren);
}

// Private

void TreeReference::setTo(const TreeReference & tr) {
  /* We cannot use (*this)==tr because tr would need to be casted to
   * TreeReference, which calls setTo and triggers an infinite loop */
  if (identifier() == tr.identifier()) {
    return;
  }
  TreeNode * currentNode = node();
  bool releaseNode = isDefined();
  if (tr.isDefined()) {
    setIdentifierAndRetain(tr.identifier());
  } else {
    m_identifier = TreePool::NoNodeIdentifier;
  }
  if (releaseNode) {
    currentNode->release();
  }
};

}
