

#include <iostream> 
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <mutex>
#include <fstream>

#define STORE_FILE "store/dumpFile"

std::mutex mtx;     // 信号量
std::string delimiter = ":";//分隔符

//节点类模板
template<typename K, typename V> 
class Node {

public:
    
    Node() {} //默认构造函数

    Node(K k, V v, int); //有参构造函数声明

    ~Node();//析构函数

    K get_key() const;   //常函数，取键值

    V get_value() const;//常函数，取实值

    void set_value(V);//设置实值
    
    // 线性数组，用于保存指向不同级别下一个节点的指针
    Node<K, V> **forward;

    int node_level;//节点_水平

private:
    K key;
    V value;
};

//有参构造函数实现
template<typename K, typename V> 
Node<K, V>::Node(const K k, const V v, int level) {
    this->key = k;
    this->value = v;
    this->node_level = level; 

    // 数组，大小为level + 1
    this->forward = new Node<K, V>*[level+1];
    
	// 用0填充数组
    memset(this->forward, 0, sizeof(Node<K, V>*)*(level+1));
};
//析构函数，释放forward数组
template<typename K, typename V> 
Node<K, V>::~Node() {
    delete []forward;
};

//实现get_key()
template<typename K, typename V> 
K Node<K, V>::get_key() const {
    return key;
};

//实现get_value()
template<typename K, typename V> 
V Node<K, V>::get_value() const {
    return value;
};

//实现set_value()
template<typename K, typename V> 
void Node<K, V>::set_value(V value) {
    this->value=value;
};

// 跳表类模板
template <typename K, typename V> 
class SkipList {

public: 
    SkipList(int);//有参构造函数
    ~SkipList();//析构
    int get_random_level();//得到随机级别
    Node<K, V>* create_node(K, V, int);//创建节点
    int insert_element(K, V);//插入元素
    void display_list();//显示列表
    bool search_element(K);//查找元素
    void delete_element(K);//删除元素
    void dump_file();//转储到文件
    void load_file();//加载文件
    int size();

private:
    //从字符串中获取键值
    void get_key_value_from_string(const std::string& str, std::string* key, std::string* value);
    //判断是否为有效字符串
    bool is_valid_string(const std::string& str);

private:    
    // 跳表的最大级别
    int _max_level;

    // 当前跳表级别 
    int _skip_list_level;

    // 指向头节点的指针 
    Node<K, V> *_header;

    // 文件读写操作
    std::ofstream _file_writer;
    std::ifstream _file_reader;

    // 跳表当前元素计数
    int _element_count;
};

// 创建新节点
template<typename K, typename V>
Node<K, V>* SkipList<K, V>::create_node(const K k, const V v, int level) {
    Node<K, V> *n = new Node<K, V>(k, v, level);
    return n;
}



template<typename K, typename V>
int SkipList<K, V>::insert_element(const K key, const V value) {
    
    mtx.lock();
    Node<K, V> *current = this->_header;

    //创建更新数组并初始化它
//update是一个数组，该数组放置了节点->forward[i]以后应该操作的节点
    Node<K, V> *update[_max_level+1];
    memset(update, 0, sizeof(Node<K, V>*)*(_max_level+1));  

   //从跳过列表的最高级别开始
    for(int i = _skip_list_level; i >= 0; i--) {
        while(current->forward[i] != NULL && current->forward[i]->get_key() < key) {
            current = current->forward[i]; 
        }
        update[i] = current;
    }

   //已达到级别0，并将指针向前移到右侧节点（需要插入密钥）。
    current = current->forward[0];

   //如果当前节点的键等于搜索到的键，则得到它
    if (current != NULL && current->get_key() == key) {
        std::cout << "key: " << key << ", exists" << std::endl;
        mtx.unlock();
        return 1;
    }

    //如果current为空，则表示我们已经到达了级别的末尾
//如果current的key不等于key，这意味着我们必须在update[0]和current节点之间插入节点
    if (current == NULL || current->get_key() != key ) {
        
        //为节点生成随机级别
        int random_level = get_random_level();

       //如果随机级别大于跳过列表的当前级别，则使用指向标题的指针初始化更新值
        if (random_level > _skip_list_level) {
            for (int i = _skip_list_level+1; i < random_level+1; i++) {
                update[i] = _header;
            }
            _skip_list_level = random_level;
        }

        //创建生成随机级别的新节点
        Node<K, V>* inserted_node = create_node(key, value, random_level);
        
       //插入节点
        for (int i = 0; i <= random_level; i++) {
            inserted_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = inserted_node;
        }
        std::cout << "Successfully inserted key:" << key << ", value:" << value << std::endl;
        _element_count ++;
    }
    mtx.unlock();
    return 0;
}

//显示跳过列表 
template<typename K, typename V> 
void SkipList<K, V>::display_list() {

    std::cout << "\n*****Skip List*****"<<"\n"; 
    for (int i = 0; i <= _skip_list_level; i++) {
        Node<K, V> *node = this->_header->forward[i]; 
        std::cout << "Level " << i << ": ";
        while (node != NULL) {
            std::cout << node->get_key() << ":" << node->get_value() << ";";
            node = node->forward[i];
        }
        std::cout << std::endl;
    }
}

//将内存中的数据转储到文件,只需输入跳表最下面的第0级
template<typename K, typename V> 
void SkipList<K, V>::dump_file() {

    std::cout << "dump_file-----------------" << std::endl;
    _file_writer.open(STORE_FILE);

    Node<K, V> *node = this->_header->forward[0]; 
    while (node != NULL) {
        _file_writer << node->get_key() << ":" << node->get_value() << "\n";
        std::cout << node->get_key() << ":" << node->get_value() << ";\n";
        node = node->forward[0];
    }

    _file_writer.flush();
    _file_writer.close();
    return ;
}

//从磁盘加载数据
//getline读出每行字符串，再以：字符，分割字符串，获得key，value
// *key = str.substr(0, str.find(delimiter));
// *value = str.substr(str.find(delimiter)+1, str.length());

template<typename K, typename V> 
void SkipList<K, V>::load_file() {

    _file_reader.open(STORE_FILE);
    std::cout << "load_file-----------------" << std::endl;
    std::string line;
    std::string* key = new std::string();
    std::string* value = new std::string();
    while (getline(_file_reader, line)) {
        get_key_value_from_string(line, key, value);
        if (key->empty() || value->empty()) {
            continue;
        }
        insert_element(*key, *value);
        std::cout << "key:" << *key << "value:" << *value << std::endl;
    }
    _file_reader.close();
}

//获取当前SkipList大小
template<typename K, typename V> 
int SkipList<K, V>::size() { 
    return _element_count;
}

template<typename K, typename V>
void SkipList<K, V>::get_key_value_from_string(const std::string& str, std::string* key, std::string* value) {

    if(!is_valid_string(str)) {
        return;
    }
    *key = str.substr(0, str.find(delimiter));
    *value = str.substr(str.find(delimiter)+1, str.length());
}

template<typename K, typename V>
bool SkipList<K, V>::is_valid_string(const std::string& str) {

    if (str.empty()) {
        return false;
    }
    if (str.find(delimiter) == std::string::npos) {
        return false;
    }
    return true;
}

//从跳过列表中删除元素
template<typename K, typename V> 
void SkipList<K, V>::delete_element(K key) {

    mtx.lock();
    Node<K, V> *current = this->_header; 
    Node<K, V> *update[_max_level+1];
    memset(update, 0, sizeof(Node<K, V>*)*(_max_level+1));

   //从跳表的最高级别开始
    for (int i = _skip_list_level; i >= 0; i--) {
        while (current->forward[i] !=NULL && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    current = current->forward[0];
    if (current != NULL && current->get_key() == key) {
       
        //从最低级别开始，删除每个级别的当前节点
        for (int i = 0; i <= _skip_list_level; i++) {

          //如果在第一级，下一个节点不是目标节点，则中断循环。
            if (update[i]->forward[i] != current) 
                break;

            update[i]->forward[i] = current->forward[i];
        }

        //删除没有元素的级别
        while (_skip_list_level > 0 && _header->forward[_skip_list_level] == 0) {
            _skip_list_level --; 
        }

        std::cout << "Successfully deleted key "<< key << std::endl;
        _element_count --;
    }
    mtx.unlock();
    return;
}


template<typename K, typename V> 
bool SkipList<K, V>::search_element(K key) {

    std::cout << "search_element-----------------" << std::endl;
    Node<K, V> *current = _header;

    //从跳过列表的最高级别开始
    for (int i = _skip_list_level; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
    }

    //已达到级别0并将指针前进到我们搜索的右侧节点
    current = current->forward[0];

    //如果当前节点的键等于搜索到的键，则得到它
    if (current and current->get_key() == key) {
        std::cout << "Found key: " << key << ", value: " << current->get_value() << std::endl;
        return true;
    }

    std::cout << "Not Found Key:" << key << std::endl;
    return false;
}

//构造跳表
template<typename K, typename V> 
SkipList<K, V>::SkipList(int max_level) {

    this->_max_level = max_level;
    this->_skip_list_level = 0;
    this->_element_count = 0;

   //创建头节点并将键和值初始化为null
    K k;
    V v;
    this->_header = new Node<K, V>(k, v, _max_level);
};

template<typename K, typename V> 
SkipList<K, V>::~SkipList() {

    if (_file_writer.is_open()) {
        _file_writer.close();
    }
    if (_file_reader.is_open()) {
        _file_reader.close();
    }
    delete _header;
}

template<typename K, typename V>
int SkipList<K, V>::get_random_level(){

    int k = 1;
    while (rand() % 2) {
        k++;
    }
    k = (k < _max_level) ? k : _max_level;
    return k;
};

