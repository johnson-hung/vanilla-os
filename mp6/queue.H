#ifndef QUEUE_H
#define QUEUE_H

#include "console.H"
#include "utils.H"

template <typename T>
struct Node{
    T* val;
    Node<T>* next;

    Node(){
        this->val = NULL;
        this->next = NULL;
    }

    Node(T* val){
        this->val = val;
        this->next = NULL;
    }
};

template <class T>
class Queue{
    private:
        int qsize; // Size of this queue
        Node<T>* head; // Pointer to the head of queue
        Node<T>* tail; // Pointer to the tail of queue

    public:
        Queue(){
            head = NULL;
            tail = NULL;
            qsize = 0;
        }

        int size(){
            // Return the current size of queue
            return qsize;
        }

        bool empty(){
            // Check if the current queue is empty
            return qsize == 0;
        }

        void enqueue(T* val){
            Node<T>* node = new Node<T>(val);
            if (empty()){
                head = node;
                tail = head;
            }
            else{
                tail->next = node;
                tail = tail->next;
            }
            qsize++;
        }

        T* dequeue(){
            if (empty()){
                return NULL;
            }

            Node<T>* rm = head;
            head = head->next;
            if (head == NULL){
                tail = head;
            }
            qsize--;
            return rm->val;
        }
};
#endif

