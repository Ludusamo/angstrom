#include <stdio.h>
#include <hashtable.h>

int main() {
	Hashtable h;
	ctor_hashtable(&h);
	set_hashtable(&h, "Potato", from_double(34));
	int i = access_hashtable(&h, "Potato").as_int32;
	printf("This should be 34: %d", i);
}
