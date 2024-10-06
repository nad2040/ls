#ifndef _LS_H_
#define _LS_H_

typedef struct fileinfo_t {
	char *mode;
	char *nlink;
	char *owner_name_or_id;
	char *group_name_or_id;
	char *file_size_or;
} fileinfo_t;


char *
get_username()
{
	return NULL;
}


#endif /* _LS_H_ */
