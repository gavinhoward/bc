#include <readline/readline.h>

char * find_pattern(char *path, char *pat, size_t patlen)
{
        DIR *dir;
        struct dirent *d;
        size_t dlen = 0;
	size_t matches = 0;
	char *match = NULL;
	char **names;
	size_t n = 0;
	size_t i = 0;
	int lever = 0;
	size_t z = 0;
	int wasadir = 0;
	
	if (!(names = malloc (sizeof(*names))))
		return NULL;
	if (!(names[n] = malloc(2560)))
		return NULL;
	names[n][0] = 0; 

        if ( ( dir = opendir(path) ) )
        {
                d = readdir(dir);
                while (d)
                {
                        dlen = strlen(d->d_name); 
                        if ( strcmp( ".", d->d_name) &&
                           ( strcmp( "..", d->d_name)) )
                        {
				strcpy(names[n], path);
				strcat(names[n], d->d_name);
				
				lever = 0;
				if (dlen < patlen)
					lever = 1;
				for (i=0; i<patlen;++i)
					if (pat[i] != d->d_name[i])
						lever = 1;
				if ( lever == 0)
				{ 
					if (match == NULL) 
					{
						match = names[n];
						if (DT_DIR == d->d_type)
							wasadir = 1;
					}
					++matches;
					if (!(names = realloc(names, sizeof(*names) * (n++ + 10))))
						return NULL;
					if (!(names[n] = malloc(256)))
						return NULL;
					names[n][0] = 0; 
				} 
                        }
                        d = readdir(dir);
                } 
		if ( matches == 1 ) 
		{
			char *str;
			for(z= 0; z < n;++z) 
			{
				if (names[z] == match)
				{
					if (!(wasadir))
						str = strdup(names[z]);
					else {
						size_t h = strlen(names[z]);
						str = malloc(h + 2);
						memcpy(str, names[z], h);
						str[h] = '/';
						str[h+ 1] = '\0';
					}
				}
				free(names[z]);
			}
			free(names);
			return str; 
		} else{
			printf("\n");
			for(z= 0; z < n;++z )
			{
				printf("%s\n", names[z]);
				free(names[z]);
			}
			free(names);
		}
        }
        closedir(dir);
        return NULL;
}

