#pragma once
#include <stdexcept>
#include <iostream>

template<typename T>
class Node {
    public:
        T data;
        Node<T>* next;
        Node<T>* prev;

    public:
        Node(T data) {
            this->data = data;
            this->next = nullptr;
            this->prev = nullptr;
        }
};

template<typename T>
class DoubleLinkedList {
    private:
        Node<T>* first;
        Node<T>* last;
        Node<T>* current;
        int length;
        int index;

    public:
        DoubleLinkedList() {
            this->length = 0;
            this->index = 0;
            this->first = nullptr;
            this->last = nullptr;
            this->current = nullptr;
        }

        T get_first() {
            return this->first->data;
        }

        T get_last() {
            return this->last->data;
        }
        
        T get() {
            return this->current->data;
        }

        T pop() {
            T data;
            
            if (length == 0) {
                throw std::out_of_range("Cannot pop from empty list");       
            } else if (length == 1) {
                data = this->current->data;
                clear();
            } else if (length > 1) {
                if (index == length - 1) {
                    data = pop_back();
                } else if (index == 0) {
                    data = pop_front();
                } else if (index > 0 && index < length - 1) {
                    Node<T>* toDelete = this->current;
                    this->current = this->current->next;
                    toDelete->prev->next = toDelete->next;
                    toDelete->next->prev = toDelete->prev;
                    data = toDelete->data;
                    delete toDelete;
                    this->length--;
                } else {
                    throw std::out_of_range("index not in range");
                }
            }

            return data;
        }

        T pop_front() {
            T data;

            if (empty()) {
                throw std::out_of_range("Cannot pop from empty list");
            }
            else if (this->length == 1) {
                data = this->first->data;
                clear();
            }
            else if (this->length > 1) {
                Node<T>* toDelete = this->first;
                data = this->first->data;
                this->first->next->prev = nullptr;
                this->first = this->first->next;

                if (this->index == 0) {
                    this->current = this->first;
                } else if (this->index > 0) {
                    this->index--;
                }

                delete toDelete;
                this->length--;
            } 
            
            return data;
        }

        T pop_back() {
            T data;

            if (empty()) {
                throw std::out_of_range("Cannot pop from empty list");
            }
            else if (this->length == 1) {
                data = this->last->data;
                clear();
            }
            else if (this->length > 1) {
                Node<T>* toDelete = this->last;
                data = this->last->data;
                this->last->prev->next = nullptr;
                this->last = this->last->prev;
                this->current = this->last;

                if (this->index == this->length - 1) {
                    this->index--;
                }

                delete toDelete;
                this->length--;
            } 
            
            return data;
        }

        void push_back(T data) {

            Node<T>* newLast = new Node<T>(data);

            if (this->last != nullptr) {
                this->last->next = newLast;
                newLast->prev = this->last;
                this->last = newLast;
            } else if (this->last == nullptr) {
                this->first = newLast;
                this->last = newLast;
                this->current = newLast;
            }
            this->length += 1;
        }

        void push_front(T data) {
            Node<T>* newFirst = new Node<T>(data);

            if (this->first != nullptr) {
                this->first->prev = newFirst;
                newFirst->next = this->first;
                this->first = newFirst;
                this->index++;
            } else if (this->first == nullptr) {
                this->first = newFirst;
                this->last = newFirst;
                this->current = newFirst;
            }
            this->length += 1;
        }

        bool set_index(int newIndex) {
            if (newIndex >= 0 && newIndex < this->length) {
                if (newIndex < this->index) {
                    for (int i = 0; i < this->index - newIndex; i++) {
                        this->current = this->current->prev;
                    }
                } else if (newIndex > this->index) {
                    for (int i = 0; i < newIndex - this->index; i++) {
                        this->current = this->current->next;
                    }
                }   

                this->index = newIndex;
                return true;
            }

            return false;
        }

        int get_index() {
            return this->index;
        }

        int get_length() {
            return this->length;
        }

        bool empty() {
            return this->length == 0;
        }

        void clear() {
            Node<T>* currentNode = first;
            while (currentNode->next != nullptr) {
                Node<T>* temp = currentNode;
                currentNode = currentNode->next;
                delete temp;
                length--;
            }
            Node<T>* temp = currentNode;
            delete temp;
            length--;

            first = nullptr;
            last = nullptr;
            current = nullptr;
            
            index = 0;
        }

        void printToCout() {
            Node<T>* currentNode = this->first;
            
            while (currentNode->next != nullptr) {
                std::cout << currentNode->data << " ";
                currentNode = currentNode->next;
            }
            std::cout << currentNode->data << std::endl;
            std::cout << "Length: " << length << "  Index: " << index << " Current Value: " << this->current->data << std::endl;
        }

        
};
