#ifndef TA_STORE_H
#define TA_STORE_H

/* install given a TA Image into secure storage */
int install_ta(const char *ta_image, size_t ta_image_len);

/* delete a TA Image corresponds to UUID from secure storage */
int delete_ta(const char *uuid_string);

#endif
