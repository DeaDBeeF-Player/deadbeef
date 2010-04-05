typedef struct vcedit_page_buffer {
	char *data;
	size_t data_len;
} vcedit_page_buffer;
	
typedef struct vcedit_buffer_chain {
	struct vcedit_buffer_chain *next;
	struct vcedit_page_buffer buffer;
} vcedit_buffer_chain;
