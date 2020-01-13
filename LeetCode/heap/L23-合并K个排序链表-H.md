# **题目**  
合并 k 个排序链表，返回合并后的排序链表。   
**示例**：  
> **输入**:  
[  
    1->4->5,  
    1->3->4,  
    2->6  
    ]  
> **输出**: 1->1->2->3->4->4->5->6
# **思路**  
合并成一个排序链表，即链表中的每个值都是递增的（此题是递增），涉及到排序，需要想到**堆结构**，所以对于本题，可以使用最小堆来完成。  
题目另外一个要求是将k个链表合并成一个，而链表又是通过指针进行连接。所以本题思路是维护一个最小堆，**堆中压入每一个链表的head**，然后进行出堆进行连接，**同时更新压入的head**，新head为，出堆head的下一个元素，最后循环直到堆为空。
# **解答**  
```
/**
 * Definition for singly-linked list.
 * struct ListNode {
 *     int val;
 *     ListNode *next;
 *     ListNode(int x) : val(x), next(NULL) {}
 * };
 */
class Solution {
public:
    struct cmp {   //按链表head的val排序
        bool operator()(ListNode* l1, ListNode* l2) {
            return l1->val > l2->val;
        }
    };
    ListNode* mergeKLists(vector<ListNode*>& lists) {
        if(lists.size() == 0) return nullptr;
        if(lists.size() == 1) return lists[0];
        
        ListNode* head = new ListNode(0);  //创建一个链表，并记录其头
        ListNode* p = head;  //后面对p进行操作
        priority_queue<ListNode*, vector<ListNode*>, cmp> que;  //最小堆
        
        for(auto iter : lists)  //每个链表head入堆
            if(iter) que.push(iter);
        
        while(!que.empty()) {
            auto node = que.top();
            que.pop();
            p->next = node; //每次取出一个堆顶的链表的head
            p = node;  //准备寻找下一个
            if(node->next)  //入堆当前链表后面的值
                que.push(node->next);
        }
        return head->next;
    }   
};
```