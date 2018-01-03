void cutilerror(char *message, int i)
{
	if ( i > 0 )
		perror("Error: ");
	fprintf(stderr, "%s", message);
	if ( i > -1 )
		exit (i);
}

void * cutilmalloc(size_t len)
{
	void *ret;
	if (!(ret = malloc(len)))
		cutilerror("cutilmalloc failed\n", 1);
	return ret;
}

void *cutilcalloc(size_t nmemb, size_t size)
{
	void *ret;
	if(!(ret = calloc(nmemb, size)))
		cutilerror("cutilcalloc failed\n", 1);
	return ret;
}

void *cutilrealloc(void *ptr, size_t size)
{
	void *ret;
	if(!(ret = realloc(ptr, size)))
		cutilerror("cutilrealloc failed\n", 1); 
	return ret;
}

void cutilfree(void *ptr)
{
	if (ptr != NULL)
		free(ptr);
	else
		cutilerror("cutilfree failed, can't free NULL pointer\n", 1); 
}


