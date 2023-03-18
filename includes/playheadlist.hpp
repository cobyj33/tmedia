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
        // PlayheadList(const PlayheadList<T>& other) {
        //     const int OTHER_STARTING_INDEX = other.get_index();
        //     other.set_index(0);
        //     for (int i = 0; i < other.get_length(); i++) {
        //         this->push_back(other.get())
        //         other.step_forward();
        //     }
        //     other.set_index(OTHER_STARTING_INDEX);
        // } 

        /**
         * @brief 
         * Guaranteed to not return NULL or nullptr (unless you yourself append NULL or nullptr to the list)
         * 
         * @return T 
         */
        T get();
        void clear();

        void clear_behind();
        void clear_ahead();

        void push_back(T item);
        void push_front(T item);

        T pop_front();

        void set_index(int index);
        void move_index(int offset);
        bool can_move_index(int offset) const;
        bool try_move_index(int offset);

        int get_index() const;
        int get_length() const;
        
        bool is_empty() const;
        bool at_end() const;
        bool at_start() const;
        bool at_edge() const;
        bool in_bounds(int index) const;

        void step_forward();
        void step_backward();
        bool try_step_forward();
        bool try_step_backward();
        bool can_step_forward() const;
        bool can_step_backward() const;
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
        currentNode = currentNode->next;

        currentNode->prev->next = nullptr;
        currentNode->prev->prev = nullptr;
        currentNode->prev = nullptr;
        this->m_length--;
    }
    currentNode->prev = nullptr;
    currentNode->next = nullptr;

    this->m_length--;
    this->m_first = nullptr;
    this->m_last = nullptr;
    this->m_current = nullptr;
    this->m_index = -1;
};

template<typename T>
void PlayheadList<T>::clear_behind() {
    if (this->m_length == 0 || this->m_index == -1) {
        return;
    }

    std::shared_ptr<PlayheadListNode<T>> currentNode = this->m_first; // starts from the start and clears until current
    int index = 0;
    while (currentNode->next != this->m_current) { // clear until current
        currentNode = currentNode->next;

        currentNode->prev->next = nullptr;
        currentNode->prev->prev = nullptr;
        currentNode->prev = nullptr;
    }

    this->m_first = this->m_current; // the current node is the new first
    this->m_current->prev = nullptr; // since the current node is the new first, the current node must have no previous node

    this->m_length -= this->m_index;
    this->m_index = 0;
}


template<typename T>
T PlayheadList<T>::pop_front() {
    if (this->m_length == 0) {
        throw std::runtime_error("Cannot pop from front of empty playhead list");
    } else if (this->m_length == 1) {
        T data = this->get();
        this->clear();
        return data;
    }

    std::shared_ptr<PlayheadListNode<T>> target_node = this->m_first;
    this->m_first->next->prev = nullptr;


    std::shared_ptr<PlayheadListNode<T>> next_first = this->m_first->next;
    this->m_first->next->prev = nullptr; // break linking on both sides
    this->m_first->next = nullptr;
    this->m_first = next_first;

    if (this->m_index == 0) { // if the start is the current
        this->m_current = next_first;
    } else if (this->m_index > 0) {
        this->m_index--;
    }

    this->m_length--;

    if (this->m_length == 1) {
        this->m_current = this->m_first;
        this->m_last = this->m_first;
    }

    return target_node->data;
}

template<typename T>
void PlayheadList<T>::clear_ahead() {
    if (this->m_length == 0 || this->m_index == this->m_length - 1) {
        return;
    }

    std::shared_ptr<PlayheadListNode<T>> currentNode = this->m_last;
    int index = this->m_index + 1;
    while (currentNode != this->m_current) { // clear backwards until current
        currentNode = currentNode->prev;

        currentNode->next->next = nullptr;
        currentNode->next->prev = nullptr;
        currentNode->next = nullptr;
    }

    this->m_current->next = nullptr;
    this->m_last = this->m_current;
    this->m_length = this->m_index + 1;
    //index does not change
}

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
    if (this->in_bounds(index)) {

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
        throw std::out_of_range("(PlayheadList<T>::set_index(int index: " + std::to_string(index) + " ))  Attempted to set out of bounds index " + std::to_string(index) + " on PlayheadList of length " + std::to_string(this->m_length));
    }
};

template<typename T>
void PlayheadList<T>::move_index(int offset) {
    if (offset == 0) { return; }
    int target_index = this->m_index + offset;
    if (this->in_bounds(target_index)) {
        this->set_index(this->m_index + offset);
    } else {
        throw std::out_of_range("(PlayheadList<T>::move_index(int offset: " + std::to_string(offset) + " )) Attempted to set out of bounds index " + std::to_string(target_index) + " on PlayheadList of length " + std::to_string(this->m_length));
    }
};

template<typename T>
bool PlayheadList<T>::can_move_index(int offset) const {
    if (offset == 0) { return true; }
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
bool PlayheadList<T>::in_bounds(int index) const {
    return index >= 0 && index < this->m_length;
};

template<typename T>
int PlayheadList<T>::get_index() const {
    return this->m_index;
};

template<typename T>
int PlayheadList<T>::get_length() const {
    return this->m_length;
};

template<typename T>
bool PlayheadList<T>::is_empty() const {
    return this->m_length == 0;
};  

template<typename T>
bool PlayheadList<T>::at_start() const {
    return this->m_length > 0 && this->m_index == 0;
};

template<typename T>
bool PlayheadList<T>::at_end() const {
    return this->m_length > 0 && this->m_index == this->m_length - 1;
};

template<typename T>
bool PlayheadList<T>::at_edge() const {
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
bool PlayheadList<T>::can_step_forward() const {
    return this->can_move_index(1);
};

template<typename T>
bool PlayheadList<T>::can_step_backward() const {
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
