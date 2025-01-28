#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>
#include <optional>

namespace BPlusTree {
static constexpr int kOrder = 4;
/**
 * @brief B+ 木のノードクラス
 * @details B+ 木のノードクラス。内部ノードと葉ノードの基底クラス
 */
class BPlusNode {
public:
    bool isLeaf_;
    
    BPlusNode(bool leaf) : isLeaf_(leaf) {}
    virtual ~BPlusNode() = default;

protected:
    static constexpr int kOrder = 4;
};

/**
 * @brief 葉ノードのクラス
 * @details B+ 木の葉ノードクラス。キーと値のペアを保持する
 */
class BPlusLeafNode : public BPlusNode {
public:
    std::vector<int> keys_;
    std::vector<int> values_;

    std::shared_ptr<BPlusLeafNode> next_;

    BPlusLeafNode() : BPlusNode(true) {}
};

/**
 * @brief 内部ノードのクラス
 * @details B+ 木の内部ノードクラス。キーと子ポインタを保持する
 */
class BPlusInternalNode : public BPlusNode {
public:
    std::vector<int> keys_;

    std::vector<std::shared_ptr<BPlusNode>> childPointers_;

    BPlusInternalNode() : BPlusNode(false) {}
};

/**
 * @brief 木構造を表現するクラス
 */
class BPlusTree {
private:
    // ルートノード
    std::shared_ptr<BPlusNode> root_;

    /**
     * @brief 木を辿り、指定された葉ノードを持つ親ノードを探す関数
     * @param key 
     * @return std::shared_ptr<BPlusLeafNode> 
     */
    std::shared_ptr<BPlusLeafNode> findLeaf(int key) {
        std::shared_ptr<BPlusNode> current = root_;
        
        while (current && !current->isLeaf_) {
            auto internalNode = std::static_pointer_cast<BPlusInternalNode>(current);
            int i = 0;
            while (i < (int)internalNode->keys_.size() && key >= internalNode->keys_[i]) {
                i++;
            }
            current = internalNode->childPointers_[i];
        }
        
        return std::static_pointer_cast<BPlusLeafNode>(current);
    }

    /**
     * @brief 葉ノードを分割し、親ノードに新たなキーを挿入する
     * @param leaf 
     */
    void splitLeafNode(std::shared_ptr<BPlusLeafNode> leaf) {
        auto newLeaf = std::make_shared<BPlusLeafNode>();

        int mid = (int)leaf->keys_.size() / 2;

        newLeaf->keys_.insert(newLeaf->keys_.end(),
                             leaf->keys_.begin() + mid, leaf->keys_.end());
        newLeaf->values_.insert(newLeaf->values_.end(),
                               leaf->values_.begin() + mid, leaf->values_.end());
        
        leaf->keys_.erase(leaf->keys_.begin() + mid, leaf->keys_.end());
        leaf->values_.erase(leaf->values_.begin() + mid, leaf->values_.end());

        newLeaf->next_ = leaf->next_;
        leaf->next_ = newLeaf;

        int newKey = newLeaf->keys_.front();

        if (leaf == root_) {
            auto newRoot_ = std::make_shared<BPlusInternalNode>();
            newRoot_->keys_.push_back(newKey);
            newRoot_->childPointers_.push_back(leaf);
            newRoot_->childPointers_.push_back(newLeaf);
            root_ = newRoot_;
        } else {
            insertInternalNode(newKey, leaf, newLeaf);
        }
    }

    /**
     * @brief 内部ノードへの挿入処理
     * @param key 
     * @param leftChild 
     * @param rightChild 
     */
    void insertInternalNode(int key, 
                            std::shared_ptr<BPlusLeafNode> leftChild,
                            std::shared_ptr<BPlusLeafNode> rightChild) {
        auto parent = findParent(root_, leftChild);
        if (!parent) {
            return;
        }
        auto internalParent = std::static_pointer_cast<BPlusInternalNode>(parent);
        int idx = 0;
        while (idx < (int)internalParent->childPointers_.size() 
               && internalParent->childPointers_[idx] != leftChild) {
            idx++;
        }
        internalParent->keys_.insert(internalParent->keys_.begin() + idx, key);
        internalParent->childPointers_.insert(internalParent->childPointers_.begin() + idx + 1, rightChild);

        if ((int)internalParent->keys_.size() >= kOrder) {
            splitInternalNode(internalParent);
        }
    }

    /**
     * @brief 内部ノードを分割し、親へ再帰的に昇格させる
     * @param internalNode 
     */
    void splitInternalNode(std::shared_ptr<BPlusInternalNode> internalNode) {
        auto newInternal = std::make_shared<BPlusInternalNode>();
    
        int midIndex = (int)internalNode->keys_.size() / 2;
        int upKey = internalNode->keys_[midIndex];

        newInternal->keys_.insert(newInternal->keys_.end(),
                                 internalNode->keys_.begin() + midIndex + 1, 
                                 internalNode->keys_.end());
        internalNode->keys_.erase(internalNode->keys_.begin() + midIndex, 
                                 internalNode->keys_.end());

        newInternal->childPointers_.insert(newInternal->childPointers_.end(),
                                          internalNode->childPointers_.begin() + midIndex + 1,
                                          internalNode->childPointers_.end());
        internalNode->childPointers_.erase(internalNode->childPointers_.begin() + midIndex + 1,
                                          internalNode->childPointers_.end());

        if (internalNode == root_) {
            auto newRoot_ = std::make_shared<BPlusInternalNode>();
            newRoot_->keys_.push_back(upKey);
            newRoot_->childPointers_.push_back(internalNode);
            newRoot_->childPointers_.push_back(newInternal);
            root_ = newRoot_;
        } else {
            insertInternalUpKey(upKey, internalNode, newInternal);
        }
    }

    /**
     * @brief 分割による昇格キーを親ノードに挿入
     * @param key 
     * @param leftChild 
     * @param rightChild 
     */
    void insertInternalUpKey(int key,
                             std::shared_ptr<BPlusInternalNode> leftChild,
                             std::shared_ptr<BPlusInternalNode> rightChild) {
        auto parent = findParent(root_, leftChild);
        if (!parent) return;

        auto internalParent = std::static_pointer_cast<BPlusInternalNode>(parent);

        int idx = 0;
        while (idx < (int)internalParent->childPointers_.size()
               && internalParent->childPointers_[idx] != leftChild) {
            idx++;
        }

        internalParent->keys_.insert(internalParent->keys_.begin() + idx, key);
        internalParent->childPointers_.insert(internalParent->childPointers_.begin() + idx + 1, rightChild);
        if ((int)internalParent->keys_.size() >= kOrder) {
            splitInternalNode(internalParent);
        }
    }

    /**
     * @brief 親ノードを探す関数
     * @param current 
     * @param child 
     * @return std::shared_ptr<BPlusNode> 
     */
    std::shared_ptr<BPlusNode> findParent(std::shared_ptr<BPlusNode> current,
                                                    std::shared_ptr<BPlusNode> child)
    {
        if (!current || current->isLeaf_) {
            return nullptr;
        }
        auto internalNode = std::static_pointer_cast<BPlusInternalNode>(current);

        for (auto ptr : internalNode->childPointers_) {
            if (ptr == child) {
                return current;
            }
            if (!ptr->isLeaf_) {
                auto candidate = findParent(ptr, child);
                if (candidate) {
                    return candidate;
                }
            }
        }
        return nullptr;
    }

public:
    BPlusTree() : root_(nullptr) {}

    /**
     * @brief キーの検索
     * @param key 
     * @return std::optional<int> 
     * @retval キーに対応する値
     * @retval キーが見つからない場合は std::nullopt
     */
    std::optional<int> search(int key) {
        if (!root_) {
            return std::nullopt;
        }
        auto leaf = findLeaf(key);
        if (!leaf) {
            return std::nullopt;
        }
        for (size_t i = 0; i < leaf->keys_.size(); i++) {
            if (leaf->keys_[i] == key) {
                return leaf->values_[i];
            }
        }
        return std::nullopt;
    }

    /**
     * @brief キーの挿入
     * @param key 
     * @param value 
     */
    
    void insert(int key, int value) {
        if (!root_) {
            auto leaf = std::make_shared<BPlusLeafNode>();
            leaf->keys_.push_back(key);
            leaf->values_.push_back(value);
            root_ = leaf;
            return;
        }

        auto leaf = findLeaf(key);
        for (size_t i = 0; i < leaf->keys_.size(); i++) {
            if (leaf->keys_[i] == key) {
                leaf->values_[i] = value;
                return;
            }
        }

        leaf->keys_.push_back(key);
        leaf->values_.push_back(value);

        for (int i = (int)leaf->keys_.size() - 1; i > 0; i--) {
            if (leaf->keys_[i] < leaf->keys_[i - 1]) {
                std::swap(leaf->keys_[i], leaf->keys_[i - 1]);
                std::swap(leaf->values_[i], leaf->values_[i - 1]);
            } else {
                break;
            }
        }

        if ((int)leaf->keys_.size() >= kOrder) {
            splitLeafNode(leaf);
        }
    }
};
} // namespace BPlussTree

int main() {
    BPlusTree::BPlusTree tree;

    // 挿入テスト
    tree.insert(10, 100);
    tree.insert(20, 200);
    tree.insert(5, 50);
    tree.insert(6, 60);
    tree.insert(15, 150);
    tree.insert(25, 250);
    tree.insert(2, 20);
    tree.insert(16, 160);
    tree.insert(18, 180);

    // 検索テスト
    for (int key : {2, 5, 6, 10, 15, 16, 18, 20, 25, 30}) {
        auto result = tree.search(key);
        if (result.has_value()) {
            std::cout << "Key " << key << " => " << result.value() << "\n";
        } else {
            std::cout << "Key " << key << " not found.\n";
        }
    }
    
    return 0;
}
