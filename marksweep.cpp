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
			/* This object was reached, so unmark it (for the next GC) and move on to
			 the next. */
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

	printf("Collected %d objects, %d remaining.\n", numObjects - vm->numObjects,
		vm->numObjects);
}


void freeVM(VM *vm) {
	vm->stackSize = 0;
	gc(vm);
	free(vm);
}



Object* newObject(VM* vm, void* data) {
	if (vm->numObjects == vm->maxObjects) {
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

void preCallFunc(VM* vm, Object* obj) {
	pushToVm(vm, obj);
}

void passPararm(VM* vm) {
	popFromVm(vm);
}

void retObject(VM* vm, Object* obj) {
	pushToVm(vm, obj);
}


void callFunc(VM* vm, Object* obj) {
	preCallFunc(vm, obj);
	//doSomething();
	passPararm(vm);
}



void callRetFunc(VM* vm, Object* obj) {
	preCallFunc(vm, obj);
	//doSomething();
	passPararm(vm);
	retObject(vm, obj);
}

void setObjectNull(Object* object) {

}

void test1() {
	VM* vm = newVM();
	int* ip = (int*)malloc(sizeof(int));
	*ip = 123;
	Object* obj1 = newObject(vm, ip);
	double* dp = (double*)malloc(sizeof(double));
	*dp = 1.23;
	Object* obj2 = newObject(vm, dp);
	freeVM(vm);
}


void test2() {
	VM* vm = newVM();
	int* p = (int*)malloc(sizeof(int));
	*p = 123;
	Object* obj = newObject(vm, p);
	gc(vm);
	callFunc(vm, obj);
	gc(vm);
}


int main(int argc, const char * argv[]) {
	test2();
	return 0;
}