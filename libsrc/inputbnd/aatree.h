#pragma once

#include <cassert>

#ifndef __cplusplus
#error unsupported language
#endif // !__cplusplus


template <typename T>
struct aatree
{
public:
#pragma push(2)
	struct aa_node
	{
		char* name;
		T* info;
		int info_array_size;
		aa_node* left;
		aa_node* right;
		aa_node* parent;
		short level;
		int visited;
	};
#pragma pop()

	aatree();
	aatree(const aatree&) = delete;
	aatree& operator=(const aatree&) = delete;
	aatree(aatree&&) = default;
	aatree& operator=(aatree&&) = default;
	~aatree();

	void Add(const char* pName, T* pInfo, int iInfoArraySize);
	int Delete(const char* pName, int iDeleteInfo, T* pInfo);
	void DeleteAll(int pDeleteInfo);
	T* Find(const char* pName);
	void ChangeInfo(const char* pName, T* pInfo, int iInfoArraySize);
	void ChangeNodeInfo(const char* pName, T* pInfo, aa_node* pCurrent, int iInfoArraySize);
	int GetNumNodes();
	void ResetVisited(aa_node* pCurrent);
	void VisitBefore(T* pInfo, aa_node* pCurrent);
	T* GetNextInOrder(char* pStr);
	void Insert(const char* pName, T* pInfo, aa_node** ppOutNode, aa_node* pParent, int iInfoArraySize);
	void ResetParents(aa_node* cur, aa_node* parent);
	int Remove(const char* pName, aa_node** ppOutNode, int iDeleteInfo, T* pInfo);
	void FreeTree(aa_node** ppOutNode, int iDeleteInfo);
	T* Search(const char* pName, aa_node* pCurrent);
	void Skew(aa_node** ppOutNode);
	void Split(aa_node** ppOutNode);
	aa_node* RotateWithLeftChild(aa_node* pNode);
	aa_node* RotateWithRightChild(aa_node* pNode);

private:
	int num_nodes;
	aa_node* root;
	aa_node* null_node;
	aa_node* cur_visited;
};

template <typename T>
inline aatree<T>::aatree()
{
	null_node = new aa_node();
	num_nodes = 0;
	null_node->level = 0;
	null_node->name = nullptr;
	null_node->info = nullptr;
	null_node->right = null_node;
	null_node->left = null_node;
	root = null_node;
}
template <typename T>
aatree<T>::~aatree()
{
	DeleteAll(1);
}

template <typename T>
void aatree<T>::Add(const char* name, T* info, int info_array_size)
{
	Insert(name, info, &root, null_node, info_array_size);
}

template <typename T>
int aatree<T>::Delete(const char* name, int delete_info, T* info)
{
	auto ret_val = Remove(name, &root, delete_info, info);
	ResetParents(root, nullptr);
	return ret_val;
}

template <typename T>
void aatree<T>::DeleteAll(int delete_info)
{
	FreeTree(&root, delete_info);
	root = null_node;
}

template <typename T>
T* aatree<T>::Find(const char* name)
{
	return Search(name, root);
}

template <typename T>
void aatree<T>::ChangeInfo(const char* pName, T* pInfo, int iInfoArraySize)
{
	ChangeNodeInfo(pName, pInfo, root, iInfoArraySize);
}

template <typename T>
void aatree<T>::ChangeNodeInfo(const char* pName, T* pInfo, aatree<T>::aa_node* pCurrent, int iInfoArraySize)
{
	if (pCurrent == null_node)
		return;

	if (strcmp(pName, pCurrent->name) == 0)
	{
		pCurrent->info = pInfo;
		pCurrent->info_array_size = iInfoArraySize;
	}
	else if (strcmp(pName, pCurrent->name) >= 0)
	{
		ChangeNodeInfo(pName, pInfo, pCurrent->right, iInfoArraySize);
	}
	else
	{
		ChangeNodeInfo(pName, pInfo, pCurrent->left, iInfoArraySize);
	}
}

template <typename T>
int aatree<T>::GetNumNodes()
{
	return num_nodes;
}

template <typename T>
void aatree<T>::ResetVisited(aatree<T>::aa_node* cur)
{
	if (!cur)
		cur = cur_visited = root;

	if (cur != null_node)
	{
		ResetVisited(cur->left);
		ResetVisited(cur->right);

		cur->visited = 0;
	}
}


template<typename T>
void aatree<T>::VisitBefore(T* info, aatree<T>::aa_node* cur)
{
	if (!cur)
		cur = cur_visited = root;

	if (cur != null_node)
	{
		VisitBefore(info, cur->left);
		if (cur->info != info)
		{
			VisitBefore(info, cur->right);
			cur->visited = 1;
		}
	}
}

template<typename T>
T* aatree<T>::GetNextInOrder(char* str)
{
	if (cur_visited == null_node)
		return nullptr;

	if (cur_visited->left != null_node && cur_visited->left->visited == 0)
	{
		cur_visited = cur_visited->left;
		return GetNextInOrder(str);
	}

	if (cur_visited->visited == 0)
	{
		cur_visited->visited = 1;
		if (str)
			strcpy(str, cur_visited->name);
		return cur_visited->info;
	}

	if (cur_visited->right != null_node && cur_visited->right->visited == 0)
	{
		cur_visited = cur_visited->right;
		return GetNextInOrder(str);
	}

	cur_visited = cur_visited->parent;

	return GetNextInOrder(str);
}

template<typename T>
void aatree<T>::Insert(const char* name, T* info, aatree<T>::aa_node** ppOutNode, aatree<T>::aa_node* parent, int info_array_size)
{
	assert(ppOutNode != nullptr);

	if (*ppOutNode == null_node)
	{
		auto new_node = new aa_node();
		new_node->name = new char[strlen(name) + 1];
		strcpy(new_node->name, name);

		new_node->info = info;
		new_node->info_array_size = info_array_size;
		new_node->right = null_node;
		new_node->left = null_node;
		new_node->parent = parent;
		new_node->level = 1;
		new_node->visited = 0;

		*ppOutNode = new_node;
		++num_nodes;

		return;
	}

	auto pOutNode = *ppOutNode;
	if (strcmp(name, pOutNode->name) <= 0)
		Insert(name, info, &pOutNode->left, pOutNode, info_array_size);
	else
		Insert(name, info, &pOutNode->right, pOutNode, info_array_size);

	Skew(ppOutNode);
	Split(ppOutNode);
}

template<typename T>
void aatree<T>::ResetParents(aatree<T>::aa_node* cur, aatree<T>::aa_node* parent)
{
	if (!parent)
		parent = null_node;

	if (cur != null_node)
	{
		cur->parent = parent;

		ResetParents(cur->left, cur);
		ResetParents(cur->right, cur);
	}
}

template<typename T>
int aatree<T>::Remove(const char* name, aatree<T>::aa_node** ppOutNode, int delete_info, T* info)
{
	static aa_node* last_ptr = nullptr;
	static aa_node* del_ptr = nullptr;
	static bool item_found = false;

	auto*& pOutNode = *ppOutNode;

	if (pOutNode == null_node)
		return 1;

	last_ptr = pOutNode;
	if (strcmp(name, pOutNode->name) >= 0)
	{
		del_ptr = pOutNode;
		Remove(name, &pOutNode->right, delete_info, info);
	}
	else
	{
		Remove(name, &pOutNode->left, delete_info, info);
	}

	if (pOutNode == last_ptr)
	{
		if (del_ptr == null_node
			|| strcmp(name, del_ptr->name)
			|| info && del_ptr->info != info)
		{
			item_found = false;
		}
		else
		{
			operator delete(del_ptr->name);
			if (del_ptr != pOutNode)
			{
				del_ptr->name = new char[strlen(pOutNode->name) + 1];
				strcpy(del_ptr->name, pOutNode->name);
				if (delete_info)
					delete del_ptr->info;

				del_ptr->info_array_size = pOutNode->info_array_size;
				del_ptr->info = pOutNode->info;
			}
			pOutNode = pOutNode->right;
			del_ptr = null_node;
			delete last_ptr;
			--num_nodes;
			item_found = true;
		}
	}
	else if (pOutNode->left->level < pOutNode->level - 1 || pOutNode->right->level < pOutNode->level - 1)
	{
		int level = pOutNode->right->level;
		if (level > --pOutNode->level)
			pOutNode->right->level = pOutNode->level;
		Skew(ppOutNode);
		Skew(&pOutNode->right);
		Skew(&pOutNode->right->right);
		Split(ppOutNode);
		Split(&pOutNode->right);
	}

	return 1;
}

template<typename T>
void aatree<T>::FreeTree(aatree<T>::aa_node** ppOutNode, int delete_info)
{
	auto*& pOutNode = *ppOutNode;
	if (pOutNode == null_node)
		return;

	FreeTree(&pOutNode->left, delete_info);
	FreeTree(&pOutNode->right, delete_info);

	delete pOutNode->name;
	delete pOutNode->info;
	delete pOutNode;

	pOutNode = null_node;
	--num_nodes;
}

template<typename T>
T* aatree<T>::Search(const char* name, aatree<T>::aa_node* cur)
{
	if (cur == null_node)
		return nullptr;

	auto cmp_res = strcmp(name, cur->name);
	if (cmp_res > 0)
		return Search(name, cur->right);

	if (cmp_res < 0)
		return Search(name, cur->left);

	cur_visited = cur;
	return cur->info;
}

template<typename T>
void aatree<T>::Skew(aatree<T>::aa_node** ppOutNode)
{
	auto*& pOutNode = *ppOutNode;
	if (pOutNode->left->level == pOutNode->level)
		pOutNode = RotateWithLeftChild(pOutNode);
}

template<typename T>
void aatree<T>::Split(aatree<T>::aa_node** ppOutNode)
{
	auto*& pOutNode = *ppOutNode;
	if (pOutNode->right->right->level == pOutNode->level)
	{
		pOutNode = RotateWithRightChild(pOutNode);
		++pOutNode->level;
	}
}

template<typename T>
typename aatree<T>::aa_node* aatree<T>::RotateWithLeftChild(aatree<T>::aa_node* pNode)
{
	auto* tmp = pNode->left;
	pNode->left = tmp->right;
	tmp->right->parent = pNode;
	tmp->right = pNode;
	tmp->parent = pNode->parent;
	pNode->parent = tmp;

	return tmp;
}

template<typename T>
typename aatree<T>::aa_node* aatree<T>::RotateWithRightChild(aatree<T>::aa_node* pNode)
{
	auto* tmp = pNode->right;
	pNode->right = tmp->left;
	tmp->left->parent = pNode;
	tmp->left = pNode;
	tmp->parent = pNode->parent;
	pNode->parent = tmp;

	return tmp;
}

