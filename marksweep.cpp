#include <stdio.h>
#include <stdlib.h>

#define STACK_MAX 256


typedef struct sObject {
	unsigned char marked;
	/* The next object in the linked list of heap allocated objects. */
	struct sObject* next;
	void* data;

} Object;

typedef struct {
	Object* stack[STACK_MAX];
	int stackSize;

	/* The first object in the linked list of all objects on the heap. */
	Object* firstObject;

	/* The total number of currently allocated objects. */
	int numObjects;

	/* The number of objects required to trigger a GC. */
	int maxObjects;
} VM;

void assert(int condition, const char* message) {
	if (!condition) {
		printf("%s\n", message);
		exit(1);
	}
}

VM* newVM() {
	VM* vm = (VM*)malloc(sizeof(VM));
	vm->stackSize = 0;
	vm->firstObject = NULL;
	vm->numObjects = 0;
	vm->maxObjects = 8;
	return vm;
}


void pushToVm(VM* vm, Object* value) {
	assert(vm->stackSize < STACK_MAX, "Stack overflow!");
	vm->stack[vm->stackSize++] = value;
}


Object* popFromVm(VM* vm) {
	assert(vm->stackSize > 0, "Stack underflow!");
	return vm->stack[--vm->stackSize];
}

void mark(Object* object) {
	if (object->marked) {
		return;
	}
	object->marked = 1;
}

void markAll(VM* vm)
{
	for (int i = 0; i < vm->stackSize; i++) {
		mark(vm->stack[i]);
	}
}

void sweep(VM* vm)
{
	Object** object = &vm->firstObject;
	while (*object) {
		if ((*object)->marked == 0) {
			/* This object wasn't reached, so remove it from the list and free it. */
			Object* unreached = *object;

			*object = unreached->next;
			if (unreached->data) {
				free(unreached->data);
			}
			free(unreached);

			vm->numObjects--;
		}
		else {
			/* This object was reached, so unmark it (for the next GC) and move on to the next. */
			(*object)->marked = 0;
			object = &(*object)->next;
		}
	}
}

void gc(VM* vm) {
	int numObjects = vm->numObjects;

	markAll(vm);
	sweep(vm);

	vm->maxObjects = vm->numObjects * 2;

	printf("Collected %d objects, %d remaining.\n", numObjects - vm->numObjects, vm->numObjects);
}


void freeVM(VM *vm) {
	vm->stackSize = 0;
	gc(vm);

	free(vm);
}



Object* newObject(VM* vm, void* data) {
	if (vm->numObjects >= vm->maxObjects) {
		gc(vm);
	}

	Object* object = (Object*)malloc(sizeof(Object));
	object->next = vm->firstObject;
	vm->firstObject = (Object*)object;
	object->marked = 0;
	object->data = data;

	vm->numObjects++;
	return object;
}




typedef void (*F)(VM* vm, Object* obj);

void callFunc(F f, VM* vm, Object* obj) {
	if (obj) {
		f(vm, obj);
	}
}




void addIntObj(VM* vm, Object* obj) {
	int* p = (int*)obj->data;
	(*p)++;
	printf("obj is:%d\n\n", *((int*)obj->data));
}


void test_call_func() {
	VM* vm = newVM();
	int* p = (int*)malloc(sizeof(int));
	*p = 111;

	{           
		Object* obj = newObject(vm, p);
		pushToVm(vm, obj);   //模拟lua，进入一段代码时先要把对象压入栈

		callFunc(addIntObj, vm, obj);

		gc(vm);              //还在栈中，此时没有回收

		callFunc(addIntObj, vm, obj);

		popFromVm(vm);       //退出代码作用域时把对象弹出栈
	}

	gc(vm);                  //不在栈中，标记不到，有回收
	freeVM(vm);
}

struct NodeB;
struct NodeA {
	int a;
	NodeB* pNb;
};

struct NodeB {
	int b;
	NodeA* pNa;
};


void test_cycle_reference() {    //测试循环引用
	NodeA* na = (NodeA*)malloc(sizeof(NodeA));
	na->a = 11;

	NodeB* nb = (NodeB*)malloc(sizeof(NodeB));
	nb->b = 22;

	na->pNb = nb;
	nb->pNa = na;

	VM* vm = newVM();
	{
		Object* obj1 = newObject(vm, na);
		pushToVm(vm, obj1);
		Object* obj2 = newObject(vm, nb);
		pushToVm(vm, obj2);


		popFromVm(vm);
		popFromVm(vm);
	}
	gc(vm);

}


int main(int argc, const char * argv[]) {
	test_call_func();

	test_cycle_reference();
	return 0;
}