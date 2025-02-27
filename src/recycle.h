/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,	   *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *									   *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael	   *
 *  Chastain, Michael Quan, and Mitchell Tse.				   *
 *									   *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc	   *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.						   *
 *									   *
 *  Much time and thought has gone into this software and you are	   *
 *  benefitting.  We hope that you share your changes too.  What goes	   *
 *  around, comes around.						   *
 ***************************************************************************/
 
/***************************************************************************
*	ROM 2.4 is copyright 1993-1998 Russ Taylor			   *
*	ROM has been brought to you by the ROM consortium		   *
*	    Russ Taylor (rtaylor@hypercube.org)				   *
*	    Gabrielle Taylor (gtaylor@hypercube.org)			   *
*	    Brian Moore (zump@rom.org)					   *
*	By using this code, you have agreed to follow the terms of the	   *
*	ROM license, in the file Rom24/doc/rom.license			   *
***************************************************************************/

/* externs */
extern char str_empty[1];
extern int mobile_count;

/* stuff for providing a crash-proof buffer */

#define MAX_BUF		16384
#define MAX_BUF_LIST 	10
#define BASE_BUF 	1024

/* valid states */
#define BUFFER_SAFE	0
#define BUFFER_OVERFLOW	1
#define BUFFER_FREED 	2

/* note recycling */
#define ND NOTE_DATA
ND	*new_note args( (void) );
void	free_note args( (NOTE_DATA *note) );
#undef ND

/* ban data recycling */
#define BD BAN_DATA
BD	*new_ban args( (void) );
void	free_ban args( (BAN_DATA *ban) );
#undef BD

/* voeter data recycling */
#define VD VOTERS_DATA
VD	*new_voter args( (void) );
void	free_voter args( (VOTERS_DATA *voter) );
#undef VD

/* member data recycling */
#define MEMBERD MEMBER_DATA
MEMBERD	*new_member args( (void) );
void	free_member args( (MEMBER_DATA *member) );
#undef MEMBERD

/* mark data recycling */
/*
#define MARKD MARK_DATA
MARKD	*new_mark args( (void) );
void	free_mark args( (MARK_DATA *member) );
#undef MARKD
*/

 
/* descriptor recycling */
#define DD DESCRIPTOR_DATA
DD	*new_descriptor args( (void) );
void	free_descriptor args( (DESCRIPTOR_DATA *d) );
#undef DD

/* char gen data recycling */
#define GD GEN_DATA
GD 	*new_gen_data args( (void) );
void	free_gen_data args( (GEN_DATA * gen) );
#undef GD

/* extra descr recycling */
#define ED EXTRA_DESCR_DATA
ED	*new_extra_descr args( (void) );
void	free_extra_descr args( (EXTRA_DESCR_DATA *ed) );
#undef ED

/* affect recycling */
#define AD AFFECT_DATA
AD	*new_affect args( (void) );
void	free_affect args( (AFFECT_DATA *af) );
#undef AD

/* object recycling */
#define OD OBJ_DATA
OD	*new_obj args( (void) );
void	free_obj args( (OBJ_DATA *obj) );
#undef OD

/* character recyling */
#define CD CHAR_DATA
#define PD PC_DATA
CD	*new_char args( (void) );
void	free_char args( (CHAR_DATA *ch) );
PD	*new_pcdata args( (void) );
void	free_pcdata args( (PC_DATA *pcdata) );
#undef PD
#undef CD


/* mob id and memory procedures */
#define MD MEM_DATA
long 	get_pc_id args( (void) );
long	get_mob_id args( (void) );
MD	*new_mem_data args( (void) );
void	free_mem_data args( ( MEM_DATA *memory) );
MD	*find_memory args( (MEM_DATA *memory, long id) );
#undef MD

/* buffer procedures */
BUFFER	*new_buf args( (void) );
BUFFER  *new_buf_size args( (int size) );
void	free_buf args( (BUFFER *buffer) );
bool	add_buf args( (BUFFER *buffer, char *string) );
void	clear_buf args( (BUFFER *buffer) );
char	*buf_string args( (BUFFER *buffer) );

HELP_AREA *	new_had		args( ( void ) );
HELP_DATA *	new_help	args( ( void ) );
void		free_help	args( ( HELP_DATA * ) );

#define STD STORE_DATA
STD    *new_store args (( void ));
void   free_store args (( STORE_DATA * store ));
#undef STD

#define STLD STORE_LIST_DATA
STLD *new_slist args ((void));
void free_slist args ((STORE_LIST_DATA *list ));
#undef STLD   

#define SOLD SOLD_LIST_DATA
SOLD *new_soldlist args ((void));
void free_soldlist args ((SOLD_LIST_DATA *list ));
#undef SOLD   

#define RD RIDDLE_DATA
RD *new_riddle args ((void));
void free_riddle args ((RIDDLE_DATA *riddle ));
#undef RD   

struct idName *newName args ((void));
void freeName args (( struct idName *name ));

ACCOUNT_DATA *new_account args ((void));
void free_account args ((ACCOUNT_DATA *account));

ACCT_PLR_DATA *new_acct_plr args (( void ));
void free_acct_plr args ((ACCT_PLR_DATA *plr_acct));

PK_DATA *newpk args (( void ));
void free_pk args (( PK_DATA *pk ));

DELAY_CODE *new_delay args (( void ));
void free_delay args (( DELAY_CODE *delay ));

