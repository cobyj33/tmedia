/**
 * @file playheadlist.hpp
 * @author Jacoby Johnson (jacobyajohnson@gmail.com)
 * @brief A Doubly-Linked List that mimicks a playhead moving up and down a "timeline"
 * @version 0.1
 * @date 2023-01-23
 * 
 * NOTES:
 * > clear method does not destruct items in the PlayheadList as of now.
 */

#ifndef PLAYHEAD_LIST_IMPLEMENTATION
#define PLAYHEAD_LIST_IMPLEMENTATION

#include <cstdlib>
#include <memory>
#include <stdexcept>

extern "C" {
    #include <libavcodec/packet.h>
}

template<typename T>
class PlayheadListNode {
    public:
        T data;
        std::shared_ptr<PlayheadListNode<T>> next;
        std::shared_ptr<PlayheadListNode<T>> prev;

        PlayheadListNode(T data) {
            this->data = data;
            this->next = nullptr;
            this->prev = nullptr;
        }
};


template<typename T>
class PlayheadList {
    private:
        std::shared_ptr<PlayheadListNode<T>> m_first;
        std::shared_ptr<PlayheadListNode<T>> m_last;
        std::shared_ptr<PlayheadListNode<T>> m_current;
        int m_length;
        int m_index;

    public:
        PlayheadList(); 

        /**
         * @brief 
         * Guaranteed to not return NULL or nullptr (unless you yourself append NULL or nullptr to the list)
         * 
         * @return T 
         */
        T get();
        void clear();
        void push_back(T item);
        void push_front(T item);
        void set_index(int index);
        void move_index(int offset);
        bool can_move_index(int offset);
        bool try_move_index(int offset);

        int get_index();
        int get_length();
        
        bool is_empty();
        bool at_end();
        bool at_start();
        bool at_edge();

        void step_forward();
        void step_backward();
        bool try_step_forward();
        bool try_step_backward();
        bool can_step_forward();
        bool can_step_backward();
};

template<typename T>
PlayheadList<T>::PlayheadList() {
    this->m_length = 0;
    this->m_index = -1;
    this->m_first = nullptr;
    this->m_last = nullptr;
    this->m_current = nullptr;
};

template<typename T>
T PlayheadList<T>::get() {
    if (this->m_current == nullptr) {
        throw std::out_of_range("Attempted to get value of PlayheadList when the current node value is nullptr. (list length: " + std::to_string(this->m_length) + ", list index: " + std::to_string(this->m_index) + " ) ");
    }
    return this->m_current->data;
};

template<typename T>
void PlayheadList<T>::clear() {
    if (this->m_length == 0) {
        return;
    }

    std::shared_ptr<PlayheadListNode<T>> currentNode = this->m_first;
    while (currentNode->next != nullptr) {
        std::shared_ptr<PlayheadListNode<T>> temp = currentNode;
        currentNode = currentNode->next;
        currentNode->prev = nullptr;
        delete temp;
        this->m_length--;
    }

    delete currentNode;

    this->m_length--;
    this->m_first = nullptr;
    this->m_last = nullptr;
    this->m_current = nullptr;
    this->m_index = -1;
};

template<typename T>
void PlayheadList<T>::push_back(T item) {
    std::shared_ptr<PlayheadListNode<T>> new_last = std::make_shared<PlayheadListNode<T>>(item);

    if (this->m_last != nullptr) {
        this->m_last->next = new_last;
        new_last->prev = this->m_last;
        this->m_last = new_last;
    } else if (this->m_last == nullptr) {
        this->m_first = new_last;
        this->m_last = new_last;
        this->m_current = new_last;
        this->m_index = 0;
    }

    this->m_length += 1;
};

template<typename T>
void PlayheadList<T>::push_front(T item) {
    std::shared_ptr<PlayheadListNode<T>> new_first = std::make_shared<PlayheadListNode<T>>(item);
    new_first->data = item;

    if (this->m_first != nullptr) {
        this->m_first->prev = new_first;
        new_first->next = this->m_first;
        this->m_first = new_first;
        this->m_index++;
    } else if (this->m_first == nullptr) {
        this->m_first = new_first;
        this->m_last = new_first;
        this->m_current = new_first;
        this->m_index = 0;
    }

    this->m_length += 1;
};

template<typename T>
void PlayheadList<T>::set_index(int index) {
    if (index >= 0 && index < this->m_length) {

        if (index < this->m_index) {
            for (int i = 0; i < this->m_index - index; i++) {
                this->m_current = this->m_current->prev;
            }
        } else if (index > this->m_index) {
            for (int i = 0; i < index - this->m_index; i++) {
                this->m_current = this->m_current->next;
            }
        }   

        this->m_index = index;
    } else {
        throw std::out_of_range("Attempted to access out of bounds index " + std::to_string(index) + " on PlayheadList of length " + std::to_string(this->m_length));
    }
};

template<typename T>
void PlayheadList<T>::move_index(int offset) {
    this->set_index(this->m_index + offset);
};

template<typename T>
bool PlayheadList<T>::can_move_index(int offset) {
    return this->m_index + offset >= 0 && this->m_index + offset < this->m_length;
};

template<typename T>
bool PlayheadList<T>::try_move_index(int offset) {
    if (this->can_move_index(offset)) {
        this->move_index(offset);
        return true;
    }
    return false;
};

template<typename T>
int PlayheadList<T>::get_index() {
    return this->m_index;
};

template<typename T>
int PlayheadList<T>::get_length() {
    return this->m_length;
};

template<typename T>
bool PlayheadList<T>::is_empty() {
    return this->m_length == 0;
};  

template<typename T>
bool PlayheadList<T>::at_start() {
    return this->m_index == 0;
};

template<typename T>
bool PlayheadList<T>::at_end() {
    return this->m_index == this->m_length - 1;
};

template<typename T>
bool PlayheadList<T>::at_edge() {
    return this->at_end() || this->at_start();
};

template<typename T>
void PlayheadList<T>::step_forward() {
    this->move_index(1);
};

template<typename T>
void PlayheadList<T>::step_backward() {
    this->move_index(-1);
};

template<typename T>
bool PlayheadList<T>::can_step_forward() {
    return this->can_move_index(1);
};

template<typename T>
bool PlayheadList<T>::can_step_backward() {
    return this->can_move_index(-1);
};

template<typename T>
bool PlayheadList<T>::try_step_forward() {
    return this->try_move_index(1);
};

template<typename T>
bool PlayheadList<T>::try_step_backward() {
    return this->try_move_index(-1);
};

#endif
