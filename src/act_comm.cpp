/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik Srfeldt, Tom Madsen, and Katja Nyboe.    *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 **************************************************************************/

/***************************************************************************
 *   ROM 2.4 is copyright 1993-1998 Russ Taylor                            *
 *   ROM has been brought to you by the ROM consortium                     *
 *       Russ Taylor (rtaylor@hypercube.org)                               *
 *       Gabrielle Taylor (gtaylor@hypercube.org)                          *
 *       Brian Moore (zump@rom.org)                                        *
 *   By using this code, you have agreed to follow the terms of the        *
 *   ROM license, in the file Rom24/doc/rom.license                        *
 **************************************************************************/

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include "merc.h"
#include "interp.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"
#include "account.h"

/* RT code to delete yourself */

void strip_argument(char *argument)
{
  char prev = '\0';
  char prev2 = '\0';
  while (*argument != '\0')
  {
    if (*argument == '$' && !isdigit(*++argument) && 
	((prev != '`') || ((prev == '`') && (prev2 == '`'))))
      *--argument = '*';
    prev2 = prev;
    prev = *argument;
    argument++;
  }
}

void check_social(CHAR_DATA *, char *, char *, long, char *);

void do_delet (CHAR_DATA * ch, char *argument)
{
    send_to_char ("You must type the full command to delete yourself.\n\r",
                  ch);
}

void do_noexp (CHAR_DATA * ch, char *argument){    if (IS_NPC (ch))    {
        send_to_char("No. You are an NPC. Quit talking and die!\n\r",ch);
	  return;    }    if (!IS_SET (ch->act2, PLR_NOEXP))    {
        SET_BIT(ch->act2, PLR_NOEXP);
        send_to_char("You will no longer get exp for kills. Remember to set this back when you want exp for kills!\n\r",ch);
        return;    }    else    {        REMOVE_BIT(ch->act2, PLR_NOEXP);
        send_to_char("You turn your ability to gain exp back on.\n\r",ch);
        return;    } }

void do_delete (CHAR_DATA * ch, char *argument)
{
    char strsave[MAX_INPUT_LENGTH];

    if (IS_NPC (ch))
        return;
    
    if (!str_cmp(ch->name,"Demandred")&& IS_IMMORTAL(ch))
    {
    	send_to_char("SCREW YOU HIPPIE!\n\r",ch);
    	return;
    }

    if (IS_DISGUISED(ch))
        REM_DISGUISE(ch);

    if (ch->pcdata->confirm_delete)
    {
        if (argument[0] != '\0')
        {
            send_to_char ("Delete status removed.\n\r", ch);
            ch->pcdata->confirm_delete = FALSE;
            return;
        }
        else
        {
            sprintf (strsave, "%s%s", PLAYER_DIR, capitalize (ch->name));
            wiznet ("$N turns $Mself into line noise.", ch, NULL, 0, 0, 0);
            stop_fighting (ch, TRUE);
            if (ch->clan > 0) 
              remove_member(ch->name, ch->clan);
            do_function (ch, &do_quit, "");
            unlink (strsave);
            return;
        }
    }

    if (argument[0] != '\0')
    {
        send_to_char ("Just type delete. No argument.\n\r", ch);
        return;
    }

    send_to_char ("Type delete again to confirm this command.\n\r", ch);
    send_to_char ("WARNING: this command is irreversible.\n\r", ch);
    send_to_char
        ("Typing delete with an argument will undo delete status.\n\r", ch);
    ch->pcdata->confirm_delete = TRUE;
    wiznet ("$N is contemplating deletion.", ch, NULL, 0, 0, get_trust (ch));
}


/* RT code to display channel status */

void do_channels (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    /* lists all channels and their status */
    send_to_char ("   channel     status\n\r", ch);
    send_to_char ("---------------------\n\r", ch);

/*    send_to_char ("gossip`*         ", ch);
    if (!IS_SET (ch->comm, COMM_NOGOSSIP))
        send_to_char ("ON\n\r", ch);
    else
        send_to_char ("OFF\n\r", ch);
*/
 /*   send_to_char("war            ",ch);
    if (!IS_SET(ch->comm,COMM_NOWAR))
      send_to_char("ON\n\r", ch);
    else
      send_to_char("OFF\n\r", ch);
*/
    send_to_char ("auction`*        ", ch);
    if (!IS_SET (ch->comm, COMM_NOAUCTION))
        send_to_char ("ON\n\r", ch);
    else
        send_to_char ("OFF\n\r", ch);

    send_to_char ("bitch  `*        ", ch);
    if (!IS_SET (ch->comm, COMM_NOBITCH))
        send_to_char ("ON\n\r", ch);
    else
        send_to_char ("OFF\n\r", ch);

    send_to_char ("MSP Music`*      ", ch);
    if (IS_SET (ch->act2, PLR_MSP_MUSIC))
        send_to_char ("ON\n\r", ch);
    else
        send_to_char ("OFF\n\r", ch);

    send_to_char ("MSP Sounds`*     ", ch);
    if (IS_SET (ch->act2, PLR_MSP_SOUND))
        send_to_char ("ON\n\r", ch);
    else
        send_to_char ("OFF\n\r", ch);

    send_to_char ("music`*          ", ch);
    if (!IS_SET (ch->comm, COMM_NOMUSIC))
        send_to_char ("ON\n\r", ch);
    else
        send_to_char ("OFF\n\r", ch);

    send_to_char ("ooc`*            ", ch);
    if (!IS_SET (ch->comm, COMM_NOOOC))
        send_to_char ("ON\n\r", ch);
    else
        send_to_char ("OFF\n\r", ch);

    send_to_char ("Quote`*          ", ch);
    if (!IS_SET (ch->comm, COMM_NOQUOTE))
        send_to_char ("ON\n\r", ch);
    else
        send_to_char ("OFF\n\r", ch);

    send_to_char ("grats`*          ", ch);
    if (!IS_SET (ch->comm, COMM_NOGRATS))
        send_to_char ("ON\n\r", ch);
    else
        send_to_char ("OFF\n\r", ch);

    send_to_char ("tournament`*     ", ch);
    if (!IS_SET (ch->comm, COMM_NOTOURNEY))
        send_to_char ("ON\n\r", ch);
    else
        send_to_char ("OFF\n\r", ch);
  
    send_to_char ("trivia`*         ", ch);
    if (!IS_SET (ch->comm, COMM_NOTRIVIA))
        send_to_char ("ON\n\r", ch);
    else
        send_to_char ("OFF\n\r", ch);
  
/*   send_to_char ("trivia`*         ", ch);
    if (!IS_SET (ch->comm, COMM_NORADIO))
        send_to_char ("ON\n\r", ch);
    else
        send_to_char ("OFF\n\r", ch);
*/
    if (IS_IMMORTAL (ch))
    {
        send_to_char ("god channel`*    ", ch);
        if (!IS_SET (ch->comm, COMM_NOWIZ))
            send_to_char ("ON\n\r", ch);
        else
            send_to_char ("OFF\n\r", ch);
    }

    send_to_char ("shouts`*         ", ch);
    if (!IS_SET (ch->comm, COMM_SHOUTSOFF))
        send_to_char ("ON\n\r", ch);
    else
        send_to_char ("OFF\n\r", ch);

    send_to_char ("tells`*          ", ch);
    if (!IS_SET (ch->comm, COMM_DEAF))
        send_to_char ("ON\n\r", ch);
    else
        send_to_char ("OFF\n\r", ch);

    send_to_char ("quiet mode`*     ", ch);
    if (IS_SET (ch->comm, COMM_QUIET))
        send_to_char ("ON\n\r", ch);
    else
        send_to_char ("OFF\n\r", ch);

    if (IS_SET (ch->comm, COMM_AFK))
        send_to_char ("You are AFK.\n\r", ch);

    if (IS_SET (ch->comm, COMM_SNOOP_PROOF))
        send_to_char ("You are immune to snooping.\n\r", ch);

    if (ch->lines != PAGELEN)
    {
        if (ch->lines)
        {
            sprintf (buf, "You display %d lines of scroll.\n\r",
                     ch->lines + 2);
            send_to_char (buf, ch);
        }
        else
            send_to_char ("Scroll buffering is off.\n\r", ch);
    }

    if (ch->prompt != NULL)
    {
        sprintf (buf, "Your current prompt is: %s\n\r", ch->prompt);
        send_to_char (buf, ch);
    }


    if (IS_SET (ch->comm, COMM_NOTELL))
        send_to_char ("You cannot use tell.\n\r", ch);

    if (IS_SET (ch->comm, COMM_NOCHANNELS))
        send_to_char ("You cannot use channels.\n\r", ch);

    if (IS_SET (ch->comm, COMM_NOEMOTE))
        send_to_char ("You cannot show emotions.\n\r", ch);

}

/* RT deaf blocks out all shouts */

void do_deaf (CHAR_DATA * ch, char *argument)
{

    if (IS_SET (ch->comm, COMM_DEAF))
    {
        send_to_char ("You can now hear tells again.\n\r", ch);
        REMOVE_BIT (ch->comm, COMM_DEAF);
    }
    else
    {
        send_to_char ("From now on, you won't hear tells.\n\r", ch);
        SET_BIT (ch->comm, COMM_DEAF);
    }
}


void do_msp (CHAR_DATA * ch, char *argument)
{

    if (argument[0] == '\0')
    {
      send_to_char("Music or Sound?\n\r", ch);
      return;
    }

    if (!str_cmp(argument, "music"))
    {
      if (IS_SET (ch->act2, PLR_MSP_MUSIC))
      {
        send_to_char ("MSP music turned off.\n\r", ch);
        REMOVE_BIT (ch->act2, PLR_MSP_MUSIC);
        REMOVE_BIT (ch->act2, PLR_MSP_PLAYING);
        send_to_char("!!MUSIC(Off)", ch);
      }
      else
      {
        send_to_char ("MSP music turned on!\n\r\n\r", ch);
        SET_BIT (ch->act2, PLR_MSP_MUSIC);
//        send_to_char("!!SOUND(wot.wav V=100 L=1 C=1 T=music U=http://www.aod.slayn.net/sounds/)", ch);
        send_to_char("!!SOUND(wot.wav V=100 L=1 C=1 U=http://aod.slayn.net/sounds/)", ch);
      }
    }
    else if (!str_cmp(argument, "sound"))
    {
      if (IS_SET (ch->act2, PLR_MSP_SOUND))
      {
        send_to_char ("MSP sounds turned off.\n\r", ch);
        REMOVE_BIT (ch->act2, PLR_MSP_SOUND);
        send_to_char("!!SOUND(Off)", ch);
      }
      else
      {
        send_to_char ("MSP sounds turned on!\n\r\n\r", ch);
        SET_BIT (ch->act2, PLR_MSP_SOUND);

        send_to_char("!!MUSIC(Sounds/mspon.mid V=100 L=1 T=MSPON)", ch);
      }
    }

}

/* RT quiet blocks out all communication */

void do_quiet (CHAR_DATA * ch, char *argument)
{
    if (IS_SET (ch->comm, COMM_QUIET))
    {
        send_to_char ("Quiet mode removed.\n\r", ch);
        REMOVE_BIT (ch->comm, COMM_QUIET);
    }
    else
    {
        send_to_char ("From now on, you will only hear says and emotes.\n\r",
                      ch);
        SET_BIT (ch->comm, COMM_QUIET);
    }
}

/* afk command */

void do_afk (CHAR_DATA * ch, char *argument)
{
    if (IS_SET (ch->comm, COMM_AFK))
    {
        send_to_char ("AFK mode removed. Type 'replay' to see tells.\n\r",
                      ch);
        act ("$n is no longer AFK.`*", ch, NULL, argument,TO_ROOM);
        REMOVE_BIT (ch->comm, COMM_AFK);
    }
    else
    {
        send_to_char ("You are now in AFK mode.\n\r", ch);
        act ("$n is now AFK.`*", ch, NULL, argument,TO_ROOM);
        SET_BIT (ch->comm, COMM_AFK);
    }
}


void do_replay (CHAR_DATA * ch, char *argument)
{
    if (IS_NPC (ch))
    {
        send_to_char ("You can't replay.\n\r", ch);
        return;
    }

    if (buf_string (ch->pcdata->buffer)[0] == '\0')
    {
        send_to_char ("You have no tells to replay.\n\r", ch);
        return;
    }

    page_to_char (buf_string (ch->pcdata->buffer), ch);
    clear_buf (ch->pcdata->buffer);
}

void do_novice( CHAR_DATA *ch, char *argument )
{
   char buf[MAX_STRING_LENGTH];
   DESCRIPTOR_DATA *d;
        
   if (!IS_SET(ch->comm, COMM_NOVICE) && ch->clan != clan_lookup("Guide"))
   {
     send_to_char("You don't have access to that channel.\n\r", ch);
     send_to_char("Ask a immortal or a guide if you really need it on.\n\r", ch);
     return;
   }

   strip_argument(argument);
   sprintf( buf, "`7You Novice: '`3%s`7'\n\r",  argument );
   send_to_char( buf, ch);

   if (ch->clan == clan_lookup("Guide"))
     sprintf(buf, "`7%s $n `7Novice: '`3$t`*'", 
		   clan_table[ch->clan].rank[ch->rank]);
   else
     sprintf(buf, "`7$n `7Novice: '`3$t`*'");

   for ( d = descriptor_list; d != NULL; d = d->next )
   {
     CHAR_DATA *victim;
     
      victim = d->original ? d->original : d->character;
       
      if ( d->connected == CON_PLAYING &&
           d->character != ch &&
           (IS_SET(victim->comm, COMM_NOVICE) ||
           victim->clan == clan_lookup("Guide")))
           
      {
        act_new( buf, ch, argument, d->character, TO_VICT,POS_SLEEPING );
      }
    }
}

void do_ooc( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
        
    if (argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOOOC))
      { 
        send_to_char("OOC channel is now ON.\n\r",ch);
        REMOVE_BIT(ch->comm,COMM_NOOOC);
      }
      else
      {
        send_to_char("OOC channel is now OFF.\n\r",ch);
        SET_BIT(ch->comm,COMM_NOOOC);
      }
    }
    else  /* gossip message sent, turn gossip on if it isn't already */
    {
        if (IS_SET(ch->comm,COMM_QUIET))
        {
          send_to_char("You must turn off quiet mode first.\n\r",ch);
          return;
        }
         
        if (IS_SET(ch->comm,COMM_NOCHANNELS))
        {
          send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
          return;
        }
       
      REMOVE_BIT(ch->comm,COMM_NOOOC);

      if (*argument == '*' && IS_IMMORTAL(ch))
      {
  	char ti[MIL];
  	argument++;
  	argument = one_argument(argument, ti);
  	sprintf(buf, "`&[`*OOC`&]->`#" );
  	check_social(ch, ti, argument, COMM_NOOOC, buf);
  	return;
      }

      sprintf( buf, "`7You OOC: '`#%s`7'\n\r",  argument );
      send_to_char( buf, ch);
      for ( d = descriptor_list; d != NULL; d = d->next )
      {
        CHAR_DATA *victim;
     
        victim = d->original ? d->original : d->character;
         
        if ( d->connected == CON_PLAYING &&
             d->character != ch &&
             !IS_SET(victim->comm,COMM_NOOOC) &&
             !IS_SET(victim->comm,COMM_QUIET) )
        {
         
          sprintf(buf, "`7%s `7OOC: '`#%s`*'\n\r", PERS(ch, victim, TRUE), argument);
          send_to_char(buf, victim);  
        }
      }
    }
}       

void do_ttalk( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
        
    if (!IS_SET(ch->act, PLR_TOURNEY) || IS_NPC(ch))
    {
      send_to_char("You aren't in a tournament.\n\r", ch);
      return;
    }
 
    strip_argument(argument);
    sprintf( buf, "`7You Tourn Talk: '`&%s`7'\n\r",  argument );
    send_to_char( buf, ch);
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
      CHAR_DATA *victim;
   
      victim = d->original ? d->original : d->character;
         
      if ( d->connected == CON_PLAYING &&
           d->character != ch &&
           (IS_SET(victim->act, PLR_TOURNAMENT_SPECT) ||
           IS_SET(victim->act, PLR_TOURNEY)))
      {
        act_new( "`7$n `7Tourn Talks: '`&$t`*'", ch, argument, d->character, TO_VICT,POS_SLEEPING );
      }
    }
}       

void do_qtalk( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
        
    if (!IS_SET(ch->act, PLR_QUESTING) || IS_NPC(ch))
    {
      send_to_char("You aren't in a quest.\n\r", ch);
      return;
    }

    strip_argument(argument);
    sprintf( buf, "`7You Quest Talk: '`&%s`7'\n\r",  argument );
    send_to_char( buf, ch);
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
      CHAR_DATA *victim;
     
      victim = d->original ? d->original : d->character;
         
      if ( d->connected == CON_PLAYING &&
           d->character != ch &&
           IS_SET(victim->act, PLR_QUESTING))
      {
        act_new( "`7$n `7Quest Talks: '`&$t`*'", ch, argument, d->character, TO_VICT,POS_SLEEPING );
      }
    }
}       

void do_bitch( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
        
    if (argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOBITCH))
      { 
        send_to_char("BITCH channel is now ON.\n\r",ch);
        REMOVE_BIT(ch->comm,COMM_NOBITCH);
      }
      else
      {
        send_to_char("BITCH channel is now OFF.\n\r",ch);
        SET_BIT(ch->comm,COMM_NOBITCH);
      }
    }
    else  /* gossip message sent, turn gossip on if it isn't already */
    {
        if (IS_SET(ch->comm,COMM_QUIET))
        {
          send_to_char("You must turn off quiet mode first.\n\r",ch);
          return;
        }
         
        if (IS_SET(ch->comm,COMM_NOCHANNELS))
        {
          send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
          return;
        }
       
      REMOVE_BIT(ch->comm,COMM_NOBITCH);

     strip_argument(argument);
     sprintf( buf, "`7You BITCH: '`3%s`7'\n\r",  argument );
      send_to_char( buf, ch);
      for ( d = descriptor_list; d != NULL; d = d->next )
      {
        CHAR_DATA *victim;
     
        victim = d->original ? d->original : d->character;
         
        if ( d->connected == CON_PLAYING &&
             d->character != ch &&
             !IS_SET(victim->comm,COMM_NOBITCH) &&
             !IS_SET(victim->comm,COMM_QUIET) )
        {
          if (!str_cmp(ch->name, "Asmodeus"))
             act_new( "`7The Bitch Queen $n: '`3$t`*'", ch, argument, d->character, TO_VICT,POS_SLEEPING );
          else if (!str_cmp(ch->name, "Dreyus"))
             act_new( "`7Mistress Debbie's little monkey man $n: '`3$t`*'", ch, argument, d->character, TO_VICT,POS_SLEEPING );
	  else if (!str_cmp(ch->name, "Thom"))
	     act_new( "`7The Bitch Princess $n: '`3$t`*'", ch, argument, d->character, TO_VICT,POS_SLEEPING );
          else if (!str_cmp(ch->name, "Jasin"))
	     act_new( "`7The Bitch Prince $n: '`3$t`*'", ch, argument, d->character, TO_VICT,POS_SLEEPING );
	  else  
             act_new( "`7Bitch $n: '`3$t`*'", ch, argument, d->character, TO_VICT,POS_SLEEPING );
        }
      }
    }
}       


void do_mintalk( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    char arg1[MIL];
    bool fAll = FALSE;
    
    if (!IS_FORSAKEN(ch) && !ch->pcdata->isMinion)
    {
      send_to_char("You serve no one.\n\r", ch);
      return;
    }

    if (IS_FORSAKEN(ch))
    {

       argument = one_argument(argument, arg1);  
  
       if (arg1[0] == '\0' || argument[0] == '\0')
       {   
         send_to_char("Minion whom what?\n\r", ch);
         return;
       }
       
       if (!str_cmp(arg1, "all"))
       {
         fAll = TRUE;
         strip_argument(argument);
         sprintf( buf, "`1You reach out to all minions: '`8%s`1'`*\n\r",
                        argument );
         send_to_char(buf, ch);
         for ( d = descriptor_list; d != NULL; d = d->next )
         {
            if ( d->connected == CON_PLAYING &&
                 d->character != ch && 
                !str_cmp(d->character->pcdata->forsaken_master, ch->name) ) 
            {
              act_new( "`1$n's voice booms in your head`*\n\r`8$t`*", 
                        ch, argument, d->character, TO_VICT, POS_SLEEPING );
            }
         }

       } 
       else
       {
         CHAR_DATA *victim;
         if ((victim = get_char_world(ch, arg1)) == NULL)
         {
           send_to_char("You can't reach them.\n\r", ch);
           return;
         }
         sprintf( buf, "`1You reach out to %s: '`8%s`1'`*\n\r",  
	                victim->name, argument );
         send_to_char(buf, ch);
         sprintf( buf, "`1%s voice booms in your head.`*\n\r`8%s`*", ch->name,
                  argument);
         send_to_char(buf, victim); 
       }       
    }
    else
    {
      sprintf(buf, "`1You speak to your master: '`8%s`1'`*\n\r", argument);
      send_to_char(buf, ch); 
    }

    for ( d = descriptor_list; d != NULL; d = d->next )
    {
       if ( d->connected == CON_PLAYING &&
            d->character != ch && 
            !str_cmp(d->character->name, ch->pcdata->forsaken_master)) 
        {
          act_new( "`1$n minions '`8$t`1'`*", ch, argument, d->character, TO_VICT,POS_SLEEPING );
        }
    }

    
}       

/* RT chat replaced with ROM gossip */
void do_gossip (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char no_col[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    send_to_char("Gossip channel no longer exists if you need help or are looking for a global channel use ooc.\n\r", ch);
    return;

    if (argument[0] == '\0')
    {
        if (IS_SET (ch->comm, COMM_NOGOSSIP))
        {
            send_to_char ("Gossip channel is now ON.\n\r", ch);
            REMOVE_BIT (ch->comm, COMM_NOGOSSIP);
        }
        else
        {
            send_to_char ("Gossip channel is now OFF.\n\r", ch);
            SET_BIT (ch->comm, COMM_NOGOSSIP);
        }
    }
    else
    {                            /* gossip message sent, turn gossip on if it isn't already */

        if (IS_SET (ch->comm, COMM_QUIET))
        {
            send_to_char ("You must turn off quiet mode first.\n\r", ch);
            return;
        }

        if (IS_SET (ch->comm, COMM_NOCHANNELS))
        {
            send_to_char
                ("The gods have revoked your channel priviliges.\n\r", ch);
            return;

        }

        REMOVE_BIT (ch->comm, COMM_NOGOSSIP);
        strip_argument(argument);
        sprintf (buf, "You gossip '`^%s`*'`*\n\r", argument);
        send_to_char (buf, ch);
        if (!IS_NPC(ch))
        {  // logging part, don't care about pretit or disguised
            colourstrip(no_col, argument);
            sprintf (buf, "`*%s gossiped '`^%s`*'",
                  ch->name,
                  no_col);
            log_special(buf, RP_TYPE);	 
            wiznet (buf, NULL, NULL, WIZ_RP, 0, 0);
        } // End logging part

/*            if (IS_NULLSTR(ch->pretit) || !str_cmp(ch->pretit, "(null)") || IS_DISGUISED(ch))
            {
                  sprintf (buf, "%s gossips '`^%s`*'`*",
               
                            IS_DISGUISED(ch)?ch->pcdata->disguise.orig_name:ch->name,
                            argument);
            }
	    else
            {
                  sprintf (buf, "%s %s gossips '`^%s `*'`*",
                  ch->pretit,
                  ch->name,
                  argument);
            }
*/                  
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            CHAR_DATA *victim;

            victim = d->original ? d->original : d->character;

            if (d->connected == CON_PLAYING &&
                d->character != ch &&
                !IS_SET (victim->comm, COMM_NOGOSSIP) &&
                !IS_SET (victim->comm, COMM_QUIET))
            {
                act_new ("$r$n gossips '`^$t`*'`*",
                          ch, argument, d->character, TO_VICT, POS_SLEEPING);
//              send_to_char(buf, victim);
//              send_to_char("\n\r", victim);

            }
        }
    }
}

/* Trivia */
void do_trivia (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        if (IS_SET (ch->comm, COMM_NOTRIVIA))
        {
            send_to_char ("Trivia channel is now ON.\n\r", ch);
            REMOVE_BIT (ch->comm, COMM_NOTRIVIA);
        }
        else
        {
            send_to_char ("Trivia channel is now OFF.\n\r", ch);
            SET_BIT (ch->comm, COMM_NOTRIVIA);
        }
    }
    else
    {                            /* gossip message sent, turn gossip on if it isn't already */

        if (IS_SET (ch->comm, COMM_QUIET))
        {
            send_to_char ("You must turn off quiet mode first.\n\r", ch);
            return;
        }

        if (IS_SET (ch->comm, COMM_NOCHANNELS))
        {
            send_to_char
                ("The gods have revoked your channel priviliges.\n\r", ch);
            return;

        }

        REMOVE_BIT (ch->comm, COMM_NOTRIVIA);
	strip_argument(argument);
        sprintf (buf, "You trivia '`!%s`*'`*\n\r", argument);
        send_to_char (buf, ch);
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            CHAR_DATA *victim;

            victim = d->original ? d->original : d->character;

            if (d->connected == CON_PLAYING &&
                d->character != ch &&
                !IS_SET (victim->comm, COMM_NOTRIVIA) &&
                !IS_SET (victim->comm, COMM_QUIET))
            {
              sprintf(buf, "`7%s trivia '`!%s`*'\n\r", PERS(ch, victim, TRUE), argument);
              send_to_char(buf, victim);  
            }
        }
    }
}
void do_radio (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        if (IS_SET (ch->comm, COMM_NORADIO))
        {
            send_to_char ("Radio channel is now ON.\n\r", ch);
            REMOVE_BIT (ch->comm, COMM_NORADIO);
        }
        else
        {
            send_to_char ("Radio channel is now OFF.\n\r", ch);
            SET_BIT (ch->comm, COMM_NORADIO);
        }
    }
    else
    {                            /* gossip message sent, turn gossip on if it isn't already */

        if (IS_SET (ch->comm, COMM_QUIET))
        {
            send_to_char ("You must turn off quiet mode first.\n\r", ch);
            return;
        }

        if (IS_SET (ch->comm, COMM_NOCHANNELS))
        {
            send_to_char
                ("The gods have revoked your channel priviliges.\n\r", ch);
            return;

        }

        REMOVE_BIT (ch->comm, COMM_NORADIO);
	strip_argument(argument);
        sprintf (buf, "You radio '`$%s`*'`*\n\r", argument);
        send_to_char (buf, ch);
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            CHAR_DATA *victim;

            victim = d->original ? d->original : d->character;

            if (d->connected == CON_PLAYING &&
                d->character != ch &&
                !IS_SET (victim->comm, COMM_NORADIO) &&
                !IS_SET (victim->comm, COMM_QUIET))
            {
                sprintf(buf, "`7%s radio: '`$%s`*'\n\r", PERS(ch, victim, TRUE), argument);
                send_to_char(buf, victim);
            }
        }
    }
}

void do_grats (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        if (IS_SET (ch->comm, COMM_NOGRATS))
        {
            send_to_char ("Grats channel is now ON.\n\r", ch);
            REMOVE_BIT (ch->comm, COMM_NOGRATS);
        }
        else
        {
            send_to_char ("Grats channel is now OFF.\n\r", ch);
            SET_BIT (ch->comm, COMM_NOGRATS);
        }
    }
    else
    {                            /* grats message sent, turn grats on if it isn't already */

        if (IS_SET (ch->comm, COMM_QUIET))
        {
            send_to_char ("You must turn off quiet mode first.\n\r", ch);
            return;
        }

        if (IS_SET (ch->comm, COMM_NOCHANNELS))
        {
            send_to_char
                ("The gods have revoked your channel priviliges.\n\r", ch);
            return;

        }

        REMOVE_BIT (ch->comm, COMM_NOGRATS);
	strip_argument(argument);
        sprintf (buf, "You grats '%s'`*\n\r", argument);
        send_to_char (buf, ch);
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            CHAR_DATA *victim;

            victim = d->original ? d->original : d->character;

            if (d->connected == CON_PLAYING &&
                d->character != ch &&
                !IS_SET (victim->comm, COMM_NOGRATS) &&
                !IS_SET (victim->comm, COMM_QUIET))
            {
                act_new ("$n grats '$t'`*",
                         ch, argument, d->character, TO_VICT, POS_SLEEPING);
            }
        }
    }
}

void do_quote (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        if (IS_SET (ch->comm, COMM_NOQUOTE))
        {
            send_to_char ("Quote channel is now ON.`*\n\r", ch);
            REMOVE_BIT (ch->comm, COMM_NOQUOTE);
        }
        else
        {
            send_to_char ("Quote channel is now OFF.`*\n\r", ch);
            SET_BIT (ch->comm, COMM_NOQUOTE);
        }
    }
    else
    {                            /* quote message sent, turn quote on if it isn't already */

        if (IS_SET (ch->comm, COMM_QUIET))
        {
            send_to_char ("You must turn off quiet mode first.\n\r", ch);
            return;
        }

        if (IS_SET (ch->comm, COMM_NOCHANNELS))
        {
            send_to_char
                ("The gods have revoked your channel priviliges.\n\r", ch);
            return;

        }

        REMOVE_BIT (ch->comm, COMM_NOQUOTE);
	strip_argument(argument);
        sprintf (buf, "You quote '`#%s`*'`*\n\r", argument);
        send_to_char (buf, ch);
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            CHAR_DATA *victim;

            victim = d->original ? d->original : d->character;

            if (d->connected == CON_PLAYING &&
                d->character != ch &&
                !IS_SET (victim->comm, COMM_NOQUOTE) &&
                !IS_SET (victim->comm, COMM_QUIET))
            {
                act_new ("$n quotes '`#$t`*'`*",
                         ch, argument, d->character, TO_VICT, POS_SLEEPING);
            }
        }
    }
}

/* RT question channel */
void do_question (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        if (IS_SET (ch->comm, COMM_NOQUESTION))
        {
            send_to_char ("Q/A channel is now ON.\n\r", ch);
            REMOVE_BIT (ch->comm, COMM_NOQUESTION);
        }
        else
        {
            send_to_char ("Q/A channel is now OFF.\n\r", ch);
            SET_BIT (ch->comm, COMM_NOQUESTION);
        }
    }
    else
    {                            /* question sent, turn Q/A on if it isn't already */

        if (IS_SET (ch->comm, COMM_QUIET))
        {
            send_to_char ("You must turn off quiet mode first.\n\r", ch);
            return;
        }

        if (IS_SET (ch->comm, COMM_NOCHANNELS))
        {
            send_to_char
                ("The gods have revoked your channel priviliges.\n\r", ch);
            return;
        }

        REMOVE_BIT (ch->comm, COMM_NOQUESTION);
	strip_argument(argument);
        sprintf (buf, "You question '`@%s`*'`*\n\r", argument);
        send_to_char (buf, ch);
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            CHAR_DATA *victim;

            victim = d->original ? d->original : d->character;

            if (d->connected == CON_PLAYING &&
                d->character != ch &&
                !IS_SET (victim->comm, COMM_NOQUESTION) &&
                !IS_SET (victim->comm, COMM_QUIET))
            {
                act_new ("$n questions '`@$t`*'`*",
                         ch, argument, d->character, TO_VICT, POS_SLEEPING);
            }
        }
    }
}

/* RT answer channel - uses same line as questions */
void do_answer (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        if (IS_SET (ch->comm, COMM_NOQUESTION))
        {
            send_to_char ("Q/A channel is now ON.\n\r", ch);
            REMOVE_BIT (ch->comm, COMM_NOQUESTION);
        }
        else
        {
            send_to_char ("Q/A channel is now OFF.\n\r", ch);
            SET_BIT (ch->comm, COMM_NOQUESTION);
        }
    }
    else
    {                            /* answer sent, turn Q/A on if it isn't already */

        if (IS_SET (ch->comm, COMM_QUIET))
        {
            send_to_char ("You must turn off quiet mode first.\n\r", ch);
            return;
        }

        if (IS_SET (ch->comm, COMM_NOCHANNELS))
        {
            send_to_char
                ("The gods have revoked your channel priviliges.\n\r", ch);
            return;
        }

        REMOVE_BIT (ch->comm, COMM_NOQUESTION);
	strip_argument(argument);
        sprintf (buf, "You answer '`&%s`*'`*\n\r", argument);
        send_to_char (buf, ch);
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            CHAR_DATA *victim;

            victim = d->original ? d->original : d->character;

            if (d->connected == CON_PLAYING &&
                d->character != ch &&
                !IS_SET (victim->comm, COMM_NOQUESTION) &&
                !IS_SET (victim->comm, COMM_QUIET))
            {
                act_new ("$n answers '`&$t`*'`*",
                         ch, argument, d->character, TO_VICT, POS_SLEEPING);
            }
        }
    }
}

/* RT music channel */
void do_music (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        if (IS_SET (ch->comm, COMM_NOMUSIC))
        {
            send_to_char ("Music channel is now ON.\n\r", ch);
            REMOVE_BIT (ch->comm, COMM_NOMUSIC);
        }
        else
        {
            send_to_char ("Music channel is now OFF.\n\r", ch);
            SET_BIT (ch->comm, COMM_NOMUSIC);
        }
    }
    else
    {                            /* music sent, turn music on if it isn't already */

        if (IS_SET (ch->comm, COMM_QUIET))
        {
            send_to_char ("You must turn off quiet mode first.\n\r", ch);
            return;
        }

        if (IS_SET (ch->comm, COMM_NOCHANNELS))
        {
            send_to_char
                ("The gods have revoked your channel priviliges.\n\r", ch);
            return;
        }

        REMOVE_BIT (ch->comm, COMM_NOMUSIC);
	strip_argument(argument);
        sprintf (buf, "You MUSIC: '`6%s`*'`*\n\r", argument);
        send_to_char (buf, ch);
        sprintf (buf, "$n MUSIC: '%s'", argument);
        for (d = descriptor_list; d != NULL; d = d->next)
        {
            CHAR_DATA *victim;

            victim = d->original ? d->original : d->character;

            if (d->connected == CON_PLAYING &&
                d->character != ch &&
                !IS_SET (victim->comm, COMM_NOMUSIC) &&
                !IS_SET (victim->comm, COMM_QUIET))
            {
                act_new ("$n MUSIC: '`6$t`*'`*",
                         ch, argument, d->character, TO_VICT, POS_SLEEPING);
            }
        }
    }
}

/* clan channels */
void do_clantalk (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if (!is_clan (ch))
    {
        send_to_char ("You aren't in a clan.\n\r", ch);
        return;
    }
    if (argument[0] == '\0')
    {
        if (IS_SET (ch->comm, COMM_NOCLAN))
        {
            send_to_char ("Clan channel is now ON\n\r", ch);
            REMOVE_BIT (ch->comm, COMM_NOCLAN);
        }
        else
        {
            send_to_char ("Clan channel is now OFF\n\r", ch);
            SET_BIT (ch->comm, COMM_NOCLAN);
        }
        return;
    }

    if (IS_SET (ch->comm, COMM_NOCHANNELS))
    {
        send_to_char ("The gods have revoked your channel priviliges.\n\r",
                      ch);
        return;
    }

    REMOVE_BIT (ch->comm, COMM_NOCLAN);

    sprintf (buf, "%s %s: '`$%s`*'`*\n\r", IS_DRAGON(ch) ? "The Dragon Reborn" : 
                  clan_table[ch->clan].rank[ch->rank], 
		  IS_DISGUISED(ch)?ch->pcdata->disguise.orig_name:ch->name, 
		  argument);
    send_to_char (buf, ch);
    for (d = descriptor_list; d != NULL; d = d->next)
    {
        if (d->connected == CON_PLAYING &&
            d->character != ch &&
            is_same_clan (ch, d->character) &&
            !IS_SET (d->character->comm, COMM_NOCLAN) &&
            !IS_SET (d->character->comm, COMM_QUIET))
        {
           send_to_char(buf, d->character);
        }
    }

    return;
}

/* clan channels */
void do_wardertalk (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;
    int color = 8;

    if (!is_clan (ch))
    {
        send_to_char ("You aren't in a clan.\n\r", ch);
        return;
    }

    if (ch->clan == clan_lookup("warder") && (ch->sex == 1))
      color = 1;
    else if (ch->clan == clan_lookup("aessedai") && (ch->sex == 2))
      color = 5;
    else
    {
      send_to_char("Huh?\n\r", ch); 
      return;
    }

    if (IS_SET (ch->comm, COMM_NOCHANNELS))
    {
        send_to_char ("The gods have revoked your channel priviliges.\n\r",
                      ch);
        return;
    }

    REMOVE_BIT (ch->comm, COMM_NOCLAN);

    sprintf (buf, "%s %s: '`%d%s`*'`*\n\r",
             clan_table[ch->clan].rank[ch->rank], 
	     IS_DISGUISED(ch)?ch->pcdata->disguise.orig_name:ch->name, 
	     color, argument);
    send_to_char (buf, ch);
    for (d = descriptor_list; d != NULL; d = d->next)
    {
        if (d->connected == CON_PLAYING &&
            d->character != ch &&
            (d->character->clan == clan_lookup ("warder") || 
             d->character->clan == clan_lookup ("aessedai")) &&            
            !IS_SET (d->character->comm, COMM_NOCLAN) &&
            !IS_SET (d->character->comm, COMM_QUIET))
        {
            victim = d->character;
            send_to_char(buf, victim);
        }
    }

    return;
}

void do_dragontalk (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;
    char color = '*';

    if (!is_clan (ch))
    {
        send_to_char ("You aren't in a clan.\n\r", ch);
        return;
    }

    if (ch->clan == clan_lookup("ashaman"))
      color = '!';
    else if (ch->clan == clan_lookup("aiel"))
      color = '&';
    else
    {
      send_to_char("Huh?\n\r", ch); 
      return;
    }

    if (IS_SET (ch->comm, COMM_NOCHANNELS))
    {
        send_to_char ("The gods have revoked your channel priviliges.\n\r",
                      ch);
        return;
    }

    REMOVE_BIT (ch->comm, COMM_NOCLAN);

    sprintf (buf, "%s %s: '`%c%s`*'`*\n\r",
             IS_DRAGON(ch) ? "Lord of the Morning" : clan_table[ch->clan].rank[ch->rank], 
               IS_DISGUISED(ch)?ch->pcdata->disguise.orig_name:ch->name, color, argument);
    send_to_char (buf, ch);
    for (d = descriptor_list; d != NULL; d = d->next)
    {
        if (d->connected == CON_PLAYING &&
            d->character != ch &&
            is_clan(d->character) && 
            (d->character->clan == clan_lookup ("ashaman") || 
             d->character->clan == clan_lookup ("aiel")))            
        {
            victim = d->character;
            send_to_char(buf, victim);
        }
    }

    return;
}


void do_immtalk (CHAR_DATA * ch, char *argument)
{
    DESCRIPTOR_DATA *d;
    CHAR_DATA *victim;
    char buf[MIL];

    if (argument[0] == '\0')
    {
        if (IS_SET (ch->comm, COMM_NOWIZ))
        {
            send_to_char ("Immortal channel is now ON\n\r", ch);
            REMOVE_BIT (ch->comm, COMM_NOWIZ);
        }
        else
        {
            send_to_char ("Immortal channel is now OFF\n\r", ch);
            SET_BIT (ch->comm, COMM_NOWIZ);
        }
        return;
    }

    REMOVE_BIT (ch->comm, COMM_NOWIZ);

if (*argument == '*')
{
  char ti[MIL];
  argument++;
  argument = one_argument(argument, ti);
  sprintf(buf, "`&[`^WIZ`&]->`*" );
  check_social(ch, ti, argument, COMM_NOWIZ, buf);
  return;
}

    strip_argument(argument);
    act_new ("`&[`^$n`&]: $t`*", ch, argument, NULL, TO_CHAR, POS_DEAD);
    for (d = descriptor_list; d != NULL; d = d->next)
    {
        if (d->connected != CON_PLAYING)
          continue;

	victim = d->original ? d->original : d->character;

	if (IS_IMMORTAL (victim) &&
            !IS_SET (victim->comm, COMM_NOWIZ))
        {
            act_new ("`&[`^$n`&]: `&$t`*", ch, argument, victim, TO_VICT,
                     POS_DEAD);
        }
    }

    return;
}

void do_imptalk (CHAR_DATA * ch, char *argument)
{
    DESCRIPTOR_DATA *d;
    char buf[MIL];

    if (argument[0] == '\0')
    {
        if (IS_SET (ch->comm, COMM_NOWIZ))
        {
            send_to_char ("Immortal channel is now ON\n\r", ch);
            REMOVE_BIT (ch->comm, COMM_NOWIZ);
        }
        else
        {
            send_to_char ("Immortal channel is now OFF\n\r", ch);
            SET_BIT (ch->comm, COMM_NOWIZ);
        }
        return;
    }

    REMOVE_BIT (ch->comm, COMM_NOWIZ);

if (*argument == '*')
{
  char ti[MIL];
  argument++;
  argument = one_argument(argument, ti);
  sprintf(buf, "`&[`!IMP`&]->`*" );
  check_social(ch, ti, argument, COMM_NOIMP, buf);
  return;
}

    strip_argument(argument);
    act_new ("`&[`1IMP `!$n`&]: $t`*", ch, argument, NULL, TO_CHAR, POS_DEAD);

    for (d = descriptor_list; d != NULL; d = d->next)
    {
      CHAR_DATA *victim;

        if (d->connected != CON_PLAYING)
	  continue;

      victim = d->original ? d->original : d->character;

	if (get_trust(victim) == MAX_LEVEL &&
          !IS_SET (victim->comm, COMM_NOWIZ))
        {
            act_new ("`&[`1IMP `!$n`&]: `&$t`*", ch, argument, victim, TO_VICT,
                     POS_DEAD);
        }
    }

    return;
}



void do_say (CHAR_DATA * ch, char *argument)
{
	DESCRIPTOR_DATA *d;
	int xp;
	char buf [MIL];
 
  if (argument[0] == '\0')
    {
        send_to_char ("Say what?\n\r", ch);
        return;
    }

  
    strip_argument(argument);
    act ("$r$n says `7'`%$T`7'`*", ch,NULL,argument,TO_ROOM);
    act ("You say `7'`%$T`7'`*", ch, NULL, argument, TO_CHAR);

    if (!IS_NPC(ch) && IS_SET(ch->act2, PLR_NORP))
      return;

    if (!IS_NPC(ch) 
	&& !IS_SET (ch->comm, COMM_AFK)
	&& count_people(ch)>1)
    {
                int len;     
		//Modifiers for the amount of xp gained in rpexp
                len = colorstrlen(argument);
                len /= 10;
		xp = len;
		if (count_people(ch)<4 && count_people(ch)>=2)
		{
			xp += (count_people(ch)/2)*3;
		}
		else if (count_people(ch)<8 
			&& count_people(ch)>=4)
		{
			xp += (count_people(ch)/2)*4;
		}
		else if (count_people(ch)>=8)
		{
			xp += (count_people(ch)/2)*5;
		}
		for (d = descriptor_list; d != NULL; d = d->next)
        {
            if (d->connected == CON_PLAYING
                && d->character == ch)
            {
                if (d->incomm[0] != '!' && strcmp (d->incomm, d->inlast))
				{
					d->repeat = 0;
				}
				else
				{
					if (++d->repeat >= 3 && d->character
					&& d->connected == CON_PLAYING)
					{
						xp=-xp*(d->repeat);
					}	
				}

            }
        }
	rpgain_exp (ch,xp);
	colourstrip(buf,argument);

     if (ch->level < LEVEL_IMMORTAL && !IS_NPC(ch))
     {
                sprintf (log_buf, "`*%s says `7'`5%s`*'`& at `3%s`* ROOM `1%ld`*", 
                        ch->name,
			buf, 
			ch->in_room->name, 
			ch->in_room->vnum);
                log_special(log_buf, RP_TYPE);
		wiznet (log_buf, NULL, NULL, WIZ_RP, 0, 0);
     }	
   }

    if (!IS_NPC (ch))
    {
        CHAR_DATA *mob, *mob_next;
        OBJ_DATA *obj, *obj_next;
        for (mob = ch->in_room->people; mob != NULL; mob = mob_next)
        {
            mob_next = mob->next_in_room;
            if (IS_NPC (mob) && HAS_TRIGGER_MOB (mob, TRIG_SPEECH)
                && mob->position == mob->pIndexData->default_pos)
                p_act_trigger (argument, mob, NULL, NULL, ch, NULL, NULL, TRIG_SPEECH);
            for ( obj = mob->carrying; obj; obj = obj_next )
            {
                obj_next = obj->next_content;
                if ( HAS_TRIGGER_OBJ( obj, TRIG_SPEECH ) )
                    p_act_trigger( argument, NULL, obj, NULL, ch, NULL, NULL, TRIG_SPEECH );          
	    }


        }
        for ( obj = ch->in_room->contents; obj; obj = obj_next )
        {
            obj_next = obj->next_content;
            if ( HAS_TRIGGER_OBJ( obj, TRIG_SPEECH ) )
                p_act_trigger( argument, NULL, obj, NULL, ch, NULL, NULL, TRIG_SPEECH );
        }

        if ( HAS_TRIGGER_ROOM( ch->in_room, TRIG_SPEECH ) )
            p_act_trigger( argument, NULL, NULL, ch->in_room, ch, NULL, NULL, TRIG_SPEECH );

    }
    return;
}

void do_osay (CHAR_DATA * ch, char *argument)
{
    
    if (ch->level < 78)
    {
	send_to_char ("Huh?\n\r", ch);
	return;
    }
    if (argument[0] == '\0')
    {
        send_to_char ("Osay what?\n\r", ch);
        return;
    }

    strip_argument(argument);
    act ("$n osays `7'`&$T`7'`*", ch, NULL, argument, TO_ROOM);
    act ("You osay `7'`&$T`7'`*", ch, NULL, argument, TO_CHAR);

    if (!IS_NPC (ch))
    {
        CHAR_DATA *mob, *mob_next;
        for (mob = ch->in_room->people; mob != NULL; mob = mob_next)
        {
            mob_next = mob->next_in_room;
            if (IS_NPC (mob) && HAS_TRIGGER_MOB (mob, TRIG_SPEECH)
                && mob->position == mob->pIndexData->default_pos)
                p_act_trigger (argument, mob, NULL, NULL, ch, NULL, NULL, TRIG_SPEECH);
        }
    }
    return;
}

void do_shout (CHAR_DATA * ch, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        if (IS_SET (ch->comm, COMM_SHOUTSOFF))
        {
            send_to_char ("You can hear shouts again.\n\r", ch);
            REMOVE_BIT (ch->comm, COMM_SHOUTSOFF);
        }
        else
        {
            send_to_char ("You will no longer hear shouts.\n\r", ch);
            SET_BIT (ch->comm, COMM_SHOUTSOFF);
        }
        return;
    }

    if (IS_SET(ch->comm,COMM_NOCHANNELS))
    {
      send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
      return;
    }

    REMOVE_BIT (ch->comm, COMM_SHOUTSOFF);

    WAIT_STATE (ch, 12);

    strip_argument(argument);
    act ("You shout '`!$T`*'", ch, NULL, argument, TO_CHAR);
    for (d = descriptor_list; d != NULL; d = d->next)
    {
        CHAR_DATA *victim;

        victim = d->original ? d->original : d->character;

        if (d->connected == CON_PLAYING &&
            d->character != ch &&
            !IS_SET (victim->comm, COMM_SHOUTSOFF) &&
            !IS_SET (victim->comm, COMM_QUIET))
        {
            act ("$n shouts '`!$t`*'", ch, argument, d->character, TO_VICT);
        }
    }

    return;
}



void do_tell (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    if (IS_SET (ch->comm, COMM_NOTELL) || IS_SET (ch->comm, COMM_DEAF))
    {
        send_to_char ("Your message didn't get through.\n\r", ch);
        return;
    }

    if (IS_SET (ch->comm, COMM_QUIET))
    {
        send_to_char ("You must turn off quiet mode first.\n\r", ch);
        return;
    }

    if (IS_SET (ch->comm, COMM_DEAF))
    {
        send_to_char ("You must turn off deaf mode first.\n\r", ch);
        return;
    }

    argument = one_argument (argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
        send_to_char ("Tell whom what?\n\r", ch);
        return;
    }

    /*
     * Can tell to PC's anywhere, but NPC's only in same room.
     * -- Furey
     */
    if ((victim = get_char_world_ooc (ch, arg)) == NULL)
    {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }

    strip_argument(argument);
    if (victim->desc == NULL && !IS_NPC (victim))
    {
        act ("$N seems to have misplaced $S link...try again later.",
             ch, NULL, victim, TO_CHAR);
        sprintf (buf, "%s tells you '2%s`*'`*\n\r", IS_DISGUISED(ch) ? ch->pcdata->disguise.orig_name : PERS (ch, victim, TRUE),
                 argument);
        buf[0] = UPPER (buf[0]);
        add_buf (victim->pcdata->buffer, buf);
        return;
    }


    if (
        (IS_SET (victim->comm, COMM_QUIET)
         || IS_SET (victim->comm, COMM_DEAF)) && 
        (!IS_IMMORTAL (ch) && !IS_NPC(ch)))
    {
        act ("$E is not receiving tells.", ch, 0, victim, TO_CHAR);
        return;
    }

    if ( !IS_NPC(victim) && !str_cmp(victim->pcdata->ignore, ch->name) && !IS_IMMORTAL (ch))
    {
        act ("$E is ignoring you.", ch, 0, victim, TO_CHAR);
        return;
    }

    if (!IS_NPC(victim) && IS_SET (victim->comm, COMM_AFK))
    {
        sprintf (buf, "%s tells you '`2%s`*'`*\n\r", IS_DISGUISED(ch)?ch->pcdata->disguise.orig_name:PERS (ch, victim, TRUE),
                 argument);
        buf[0] = UPPER (buf[0]);
        add_buf (victim->pcdata->buffer, buf);
    }
    

    sprintf (buf, "You tell %s '`2%s`*'`*\n\r", IS_DISGUISED(victim) ? victim->pcdata->disguise.orig_name : PERS(victim, ch, TRUE), argument);
    send_to_char(buf, ch);
    sprintf (buf, "%s tells you '`2%s`*'`*\n\r", IS_DISGUISED(ch)?ch->pcdata->disguise.orig_name:PERS (ch, victim, TRUE),
                 argument);
        buf[0] = UPPER (buf[0]);
    send_to_char(buf,victim);
    victim->oreply = ch;

    if (!IS_NPC (ch) && IS_NPC (victim) && HAS_TRIGGER_MOB (victim, TRIG_SPEECH))
        p_act_trigger (argument, victim, NULL, NULL, ch, NULL, NULL, TRIG_SPEECH);

    return;
}

void do_whisper (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    if (IS_SET (ch->comm, COMM_NOTELL) || IS_SET (ch->comm, COMM_DEAF))
    {
        send_to_char ("Your message didn't get through.\n\r", ch);
        return;
    }

    if (IS_SET (ch->comm, COMM_QUIET))
    {
        send_to_char ("You must turn off quiet mode first.\n\r", ch);
        return;
    }

    if (IS_SET (ch->comm, COMM_DEAF))
    {
        send_to_char ("You must turn off deaf mode first.\n\r", ch);
        return;
    }

    argument = one_argument (argument, arg);

    if (arg[0] == '\0' || argument[0] == '\0')
    {
        send_to_char ("Whisper whom what?\n\r", ch);
        return;
    }

    /*
     * Can tell to PC's anywhere, but NPC's only in same room.
     * -- Furey
     */
    if ((victim = get_char_room (ch, NULL, arg)) == NULL)
    {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }

    if (victim->desc == NULL && !IS_NPC (victim))
    {
        act ("$N seems to have misplaced $S link...try again later.",
             ch, NULL, victim, TO_CHAR);
        sprintf (buf, "%s whispers to you '2%s`*'`*\n\r", IS_DISGUISED(ch) ? ch->pcdata->disguise.orig_name : PERS (ch, victim, TRUE),
                 argument);
        buf[0] = UPPER (buf[0]);
        add_buf (victim->pcdata->buffer, buf);
        return;
    }


    if (
        (IS_SET (victim->comm, COMM_QUIET)
         || IS_SET (victim->comm, COMM_DEAF)) && 
        (!IS_IMMORTAL (ch) && !IS_NPC(ch)))
    {
        act ("$E is not listening.", ch, 0, victim, TO_CHAR);
        return;
    }

    if ( !IS_NPC(victim) && !str_cmp(victim->pcdata->ignore, ch->name) && !IS_IMMORTAL (ch))
    {
        act ("$E is ignoring you.", ch, 0, victim, TO_CHAR);
        return;
    }

    if (!IS_NPC(ch) 
	&& !IS_SET (ch->comm, COMM_AFK)
	&& count_people(ch)>1)
    {
                int len, xp;     
		//Modifiers for the amount of xp gained in rpexp
                len = colorstrlen(argument);
                len /= 10;
		xp = len;
                if (ch->desc->incomm[0] != '!' && 
                   strcmp (ch->desc->incomm, ch->desc->inlast))
		{
		  ch->desc->repeat = 0;
		}
		else
		{
		  if (++ch->desc->repeat >= 3 && ch->desc->character
		     && ch->desc->connected == CON_PLAYING)
		  {
		     xp=-xp*(ch->desc->repeat);
		  }	
		}
      rpgain_exp (ch,xp);
    }


    if (!IS_NPC(ch))
    {  // logging part, don't care about pretit or disguised
	 char no_col[MSL];
         colourstrip(no_col, argument);
         sprintf (buf, "`*%s whispers %s '`2%s`*'",
                  ch->name,
                  victim->name,
                  no_col);
          log_special(buf, RP_TYPE);
          wiznet (buf, NULL, NULL, WIZ_RP, 0, 0);
    } // End logging part

    if (!IS_NPC(victim) && IS_SET (victim->comm, COMM_AFK))
    {
        sprintf (buf, "%s whispers to you '`2%s`*'`*\n\r", IS_DISGUISED(ch)?ch->pcdata->disguise.orig_name:PERS (ch, victim, FALSE),
                 argument);
        buf[0] = UPPER (buf[0]);
        add_buf (victim->pcdata->buffer, buf);
    }
    

    sprintf (buf, "You whisper to %s '`2%s`*'`*\n\r", IS_DISGUISED(victim) ? victim->pcdata->disguise.orig_name : PERS(victim, ch, FALSE), argument);
    send_to_char(buf, ch);
    sprintf (buf, "%s whispers to you '`2%s`*'`*\n\r", IS_DISGUISED(ch)?ch->pcdata->disguise.orig_name:PERS (ch, victim, FALSE),
                 argument);
        buf[0] = UPPER (buf[0]);
    send_to_char(buf,victim);
    victim->ireply = ch;

    if (!IS_NPC (ch) && IS_NPC (victim) && HAS_TRIGGER_MOB (victim, TRIG_SPEECH))
        p_act_trigger (argument, victim, NULL, NULL, ch, NULL, NULL, TRIG_SPEECH);

    return;
}

void do_htell(CHAR_DATA * ch, char *argument)
{
  send_to_char("Tell has been divided into two different commands itell and tell.\n\r", ch);
  send_to_char("Use itell for ic conversations and tell for ooc conversations.\n\r", ch);
  return;
}

void do_hreply(CHAR_DATA * ch, char *argument)
{
  send_to_char("reply has been divided into two different commands ireply and oreply.\n\r", ch);
  send_to_char("Use ireply for ic conversations and oreply for ooc conversations.\n\r", ch);
  return;
}

void do_ignore(CHAR_DATA * ch, char *argument)
{
  char buf[MSL];

  if (IS_NPC(ch))
  {
    send_to_char("Not on NPCs\n\r", ch);
    return;
  }

  if (argument[0] == '\0')
  {
    send_to_char("Inore whom?\n\r", ch);
    return;
  }
 
  free_string(ch->pcdata->ignore);
  ch->pcdata->ignore = str_dup(argument);
  sprintf(buf, "Any tells from %s will now be ignored.\n\r", argument);
  send_to_char(buf, ch);
  
}  

void do_ireply (CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];

    if (IS_SET (ch->comm, COMM_NOTELL))
    {
        send_to_char ("Your message didn't get through.\n\r", ch);
        return;
    }

    if ((victim = ch->ireply) == NULL || victim->in_room != ch->in_room)
    {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }

    if (victim->desc == NULL && !IS_NPC (victim))
    {
        act ("$N seems to have misplaced $S link...try again later.",
             ch, NULL, victim, TO_CHAR);
        sprintf (buf, "%s whispers to you '`2%s`*'`*\n\r", IS_DISGUISED(ch)?ch->pcdata->disguise.orig_name:PERS (ch, victim, FALSE),
                 argument);
        buf[0] = UPPER (buf[0]);
        add_buf (victim->pcdata->buffer, buf);
        return;
    }

    if (
        (IS_SET (victim->comm, COMM_QUIET)
         || IS_SET (victim->comm, COMM_DEAF)) && !IS_IMMORTAL (ch)
        && !IS_IMMORTAL (victim))
    {
        act_new ("$E is not listening.", ch, 0, victim, TO_CHAR,
                 POS_DEAD);
        return;
    }

    if (!IS_NPC(ch))
    {  // logging part, don't care about pretit or disguised
	 char no_col[MSL];
         colourstrip(no_col, argument);
         sprintf (buf, "`*%s whispers to %s '`2%s`*'",
                  ch->name,
                  victim->name,
                  no_col);
          log_special(buf, RP_TYPE);
          wiznet (buf, NULL, NULL, WIZ_RP, 0, 0);
    } // End logging part

    if (IS_SET (victim->comm, COMM_AFK))
    {
        if (!IS_NPC(victim)) 
        {
          sprintf (buf, "%s whispers you '`2%s`*'`*\n\r", IS_DISGUISED(ch)?ch->pcdata->disguise.orig_name:PERS (ch, victim, FALSE),
                 argument);
          buf[0] = UPPER (buf[0]);
          add_buf (victim->pcdata->buffer, buf);
        }
    }

    sprintf (buf, "You whisper to %s '`2%s`*'`*\n\r", IS_DISGUISED(victim) ? victim->pcdata->disguise.orig_name : PERS(victim, ch, FALSE), argument);
    send_to_char(buf, ch);
    sprintf (buf, "%s whispers to you '`2%s`*'`*\n\r", IS_DISGUISED(ch)?ch->pcdata->disguise.orig_name:PERS (ch, victim, FALSE),
                 argument);
        buf[0] = UPPER (buf[0]);
    send_to_char(buf,victim);
    victim->ireply = ch;

    return;
}

void do_oreply (CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];

    if (IS_SET (ch->comm, COMM_NOTELL))
    {
        send_to_char ("Your message didn't get through.\n\r", ch);
        return;
    }

    if ((victim = ch->oreply) == NULL)
    {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }

    if ( !IS_NPC(victim) && !str_cmp(victim->pcdata->ignore, ch->name) && !IS_IMMORTAL (ch))
    {
        act ("$E is ignoring you.", ch, 0, victim, TO_CHAR);
        return;
    }

    if (victim->desc == NULL && !IS_NPC (victim))
    {
        act ("$N seems to have misplaced $S link...try again later.",
             ch, NULL, victim, TO_CHAR);
        sprintf (buf, "%s tells you '`2%s`*'`*\n\r", IS_DISGUISED(ch)?ch->pcdata->disguise.orig_name:PERS (ch, victim, TRUE),
                 argument);
        buf[0] = UPPER (buf[0]);
        add_buf (victim->pcdata->buffer, buf);
        return;
    }

/*    if (!IS_IMMORTAL (ch) && !IS_AWAKE (victim))
    {
        act ("$E can't hear you.", ch, 0, victim, TO_CHAR);
        return;
    }
*/
    if (
        (IS_SET (victim->comm, COMM_QUIET)
         || IS_SET (victim->comm, COMM_DEAF)) && !IS_IMMORTAL (ch)
        && !IS_IMMORTAL (victim))
    {
        act_new ("$E is not receiving tells.", ch, 0, victim, TO_CHAR,
                 POS_DEAD);
        return;
    }

/*    if (!IS_IMMORTAL (victim) && !IS_AWAKE (ch))
    {
        send_to_char ("In your dreams, or what?\n\r", ch);
        return;
    }
*/
    if (IS_SET (victim->comm, COMM_AFK))
    {
        if (!IS_NPC(victim)) 
        {
          sprintf (buf, "%s tells you '`2%s`*'`*\n\r", IS_DISGUISED(ch)?ch->pcdata->disguise.orig_name:PERS (ch, victim, TRUE),
                 argument);
          buf[0] = UPPER (buf[0]);
          add_buf (victim->pcdata->buffer, buf);
        }
    }

    sprintf (buf, "You tell %s '`2%s`*'`*\n\r", IS_DISGUISED(victim) ? victim->pcdata->disguise.orig_name : PERS(victim, ch, TRUE), argument);
    send_to_char(buf, ch);
    sprintf (buf, "%s tells you '`2%s`*'`*\n\r", IS_DISGUISED(ch)?ch->pcdata->disguise.orig_name:PERS (ch, victim, TRUE),
                 argument);
        buf[0] = UPPER (buf[0]);
    send_to_char(buf,victim);
    victim->oreply = ch;

    return;
}



void do_yell (CHAR_DATA * ch, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
        send_to_char ("Yell what?\n\r", ch);
        return;
    }

    strip_argument(argument);
    act ("You yell '`1$t`*'", ch, argument, NULL, TO_CHAR);
    for (d = descriptor_list; d != NULL; d = d->next)
    {
        if (d->connected == CON_PLAYING
            && d->character != ch
            && d->character->in_room != NULL
            && d->character->in_room->area == ch->in_room->area
            && !IS_SET (d->character->comm, COMM_QUIET))
        {
            act ("$n yells '`1$t`*'", ch, argument, d->character, TO_VICT);
        }
    }

    return;
}


void do_emote (CHAR_DATA * ch, char *argument)
{
	DESCRIPTOR_DATA *d;
	int xp;
	char buf [MIL];
    if (!IS_NPC (ch) && IS_SET (ch->comm, COMM_NOEMOTE))
    {
        send_to_char ("You can't show your emotions.\n\r", ch);
        return;
    }

    if (argument[0] == '\0')
    {
        send_to_char ("Emote what?\n\r", ch);
        return;
    }

    if (argument[0] != '\'')
    {
      sprintf(buf, " %s", argument);
    }
    else
    {
      sprintf(buf, "%s", argument);
    }
    MOBtrigger = FALSE;
    act ("$n$T", ch, NULL, buf, TO_ROOM);
    act ("$n$T", ch, NULL, buf, TO_CHAR);
    MOBtrigger = TRUE;
    if (!IS_NPC(ch) && IS_SET(ch->act2, PLR_NORP))
      return;

	if (!IS_NPC(ch) 
		&& !IS_SET (ch->comm, COMM_AFK)
		&& count_people(ch)>1)
	{
		//Modifiers for the amount of xp gained in rpexp
                int len;     
		//Modifiers for the amount of xp gained in rpexp
                len = colorstrlen(argument);
                len /= 15;
		xp = len;
		if (count_people(ch)<4
			&& count_people(ch)>=2)
		{
			xp += (count_people(ch)/2)*3;
		}
		else if (count_people(ch)<8
			&& count_people(ch)>=4)
		{
			xp += (count_people(ch)/2)*4;
		}
		else if (count_people(ch)>=8)
		{
			xp += (count_people(ch)/2)*5;
		}
		for (d = descriptor_list; d != NULL; d = d->next)
        {
            if (d->connected == CON_PLAYING
                && d->character == ch)
            {
                if (d->incomm[0] != '!' && strcmp (d->incomm, d->inlast))
				{
					d->repeat = 0;
				}
				else
				{
					if (++d->repeat >= 3 && d->character
					&& d->connected == CON_PLAYING)
					{
						xp=-xp*(d->repeat);
					}	
				}

            }
        }
		rpgain_exp (ch,xp);
		colourstrip(buf,argument);
		sprintf (log_buf, "%s `7%s`& at `3%s`* ROOM `1%ld'`*",
			ch->name, 
			buf, 
			ch->in_room->name, 
			ch->in_room->vnum);
                log_special(log_buf, RP_TYPE);
		wiznet (log_buf, NULL, NULL, WIZ_RP, 0, 0);

	}
    return;
}
void do_think (CHAR_DATA * ch, char *argument)
{
	DESCRIPTOR_DATA *d;
	int xp;
	char buf [MIL];
    if (!IS_NPC (ch) && IS_SET (ch->comm, COMM_NOEMOTE))
    {
        send_to_char ("You can't show your thoughts.\n\r", ch);
        return;
    }

    if (argument[0] == '\0')
    {
        send_to_char ("Think what?\n\r", ch);
        return;
    }

    
    sprintf(buf, "%s", argument);
    
    act ("You think to yourself '`4$T`*'", ch, NULL, buf, TO_CHAR);
    if (!IS_NPC(ch) && IS_SET(ch->act2, PLR_NORP))
      return;

	if (!IS_NPC(ch) 
		&& !IS_SET (ch->comm, COMM_AFK))
	{
		(number_percent()>50)?xp =1:xp =0;
		for (d = descriptor_list; d != NULL; d = d->next)
        {
            if (d->connected == CON_PLAYING
                && d->character == ch)
            {
                if (d->incomm[0] != '!' && strcmp (d->incomm, d->inlast))
				{
					d->repeat = 0;
				}
				else
				{
					if (++d->repeat >= 3 && d->character
					&& d->connected == CON_PLAYING)
					{
						xp=-xp*(d->repeat);
					}	
				}

            }
        }
		rpgain_exp (ch,xp);
		colourstrip(buf,argument);
		sprintf (log_buf, "%s thinks '`4%s`*'`& at `3%s`* ROOM `1%ld'`*",
			ch->name, 
			buf, 
			ch->in_room->name, 
			ch->in_room->vnum);
                log_special(log_buf, RP_TYPE);
		wiznet (log_buf, NULL, NULL, WIZ_RP, 0, 0);

	}
    return;
}


void do_pmote (CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *vch;
    char *letter;
    char *name;
    char last[MAX_INPUT_LENGTH], temp[MAX_STRING_LENGTH];
    int matches = 0;

    if (!IS_NPC (ch) && IS_SET (ch->comm, COMM_NOEMOTE))
    {
        send_to_char ("You can't show your emotions.\n\r", ch);
        return;
    }

    if (argument[0] == '\0')
    {
        send_to_char ("Emote what?\n\r", ch);
        return;
    }

    strip_argument(argument);
    act ("$n $t", ch, argument, NULL, TO_CHAR);

    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
        if (vch->desc == NULL || vch == ch)
            continue;

        if ((letter = strstr (argument, vch->name)) == NULL)
        {
            MOBtrigger = FALSE;
            act ("$N $t", vch, argument, ch, TO_CHAR);
            MOBtrigger = TRUE;
            continue;
        }

        strcpy (temp, argument);
        temp[strlen (argument) - strlen (letter)] = '\0';
        last[0] = '\0';
        name = vch->name;

        for (; *letter != '\0'; letter++)
        {
            if (*letter == '\'' && matches == int(strlen (vch->name)))
            {
                strcat (temp, "r");
                continue;
            }

            if (*letter == 's' && matches == int(strlen (vch->name)))
            {
                matches = 0;
                continue;
            }

            if (matches == int(strlen (vch->name)))
            {
                matches = 0;
            }

            if (*letter == *name)
            {
                matches++;
                name++;
                if (matches == int(strlen (vch->name)))
                {
                    strcat (temp, "you");
                    last[0] = '\0';
                    name = vch->name;
                    continue;
                }
                strncat (last, letter, 1);
                continue;
            }

            matches = 0;
            strcat (temp, last);
            strncat (temp, letter, 1);
            last[0] = '\0';
            name = vch->name;
        }

        MOBtrigger = FALSE;
        act ("$N $t", vch, temp, ch, TO_CHAR);
        MOBtrigger = TRUE;
    }

    return;
}


/*
 * All the posing stuff.
 */
struct pose_table_type {
    char *message[2 * MAX_CLASS];
};

const struct pose_table_type pose_table[] = {
    {
     {
      "You sizzle with energy.",
      "$n sizzles with energy.",
      "You feel very holy.",
      "$n looks very holy.",
      "You perform a small card trick.",
      "$n performs a small card trick.",
      "You show your bulging muscles.",
      "$n shows $s bulging muscles."}
     },

    {
     {
      "You turn into a butterfly, then return to your normal shape.",
      "$n turns into a butterfly, then returns to $s normal shape.",
      "You nonchalantly turn wine into water.",
      "$n nonchalantly turns wine into water.",
      "You wiggle your ears alternately.",
      "$n wiggles $s ears alternately.",
      "You crack nuts between your fingers.",
      "$n cracks nuts between $s fingers."}
     },

    {
     {
      "Blue sparks fly from your fingers.",
      "Blue sparks fly from $n's fingers.",
      "A halo appears over your head.",
      "A halo appears over $n's head.",
      "You nimbly tie yourself into a knot.",
      "$n nimbly ties $mself into a knot.",
      "You grizzle your teeth and look mean.",
      "$n grizzles $s teeth and looks mean."}
     },

    {
     {
      "Little red lights dance in your eyes.",
      "Little red lights dance in $n's eyes.",
      "You recite words of wisdom.",
      "$n recites words of wisdom.",
      "You juggle with daggers, apples, and eyeballs.",
      "$n juggles with daggers, apples, and eyeballs.",
      "You hit your head, and your eyes roll.",
      "$n hits $s head, and $s eyes roll."}
     },

    {
     {
      "A slimy green monster appears before you and bows.",
      "A slimy green monster appears before $n and bows.",
      "Deep in prayer, you levitate.",
      "Deep in prayer, $n levitates.",
      "You steal the underwear off every person in the room.",
      "Your underwear is gone!  $n stole it!",
      "Crunch, crunch -- you munch a bottle.",
      "Crunch, crunch -- $n munches a bottle."}
     },

    {
     {
      "You turn everybody into a little pink elephant.",
      "You are turned into a little pink elephant by $n.",
      "An angel consults you.",
      "An angel consults $n.",
      "The dice roll ... and you win again.",
      "The dice roll ... and $n wins again.",
      "... 98, 99, 100 ... you do pushups.",
      "... 98, 99, 100 ... $n does pushups."}
     },

    {
     {
      "A small ball of light dances on your fingertips.",
      "A small ball of light dances on $n's fingertips.",
      "Your body glows with an unearthly light.",
      "$n's body glows with an unearthly light.",
      "You count the money in everyone's pockets.",
      "Check your money, $n is counting it.",
      "Arnold Schwarzenegger admires your physique.",
      "Arnold Schwarzenegger admires $n's physique."}
     },

    {
     {
      "Smoke and fumes leak from your nostrils.",
      "Smoke and fumes leak from $n's nostrils.",
      "A spot light hits you.",
      "A spot light hits $n.",
      "You balance a pocket knife on your tongue.",
      "$n balances a pocket knife on your tongue.",
      "Watch your feet, you are juggling granite boulders.",
      "Watch your feet, $n is juggling granite boulders."}
     },

    {
     {
      "The light flickers as you rap in magical languages.",
      "The light flickers as $n raps in magical languages.",
      "Everyone levitates as you pray.",
      "You levitate as $n prays.",
      "You produce a coin from everyone's ear.",
      "$n produces a coin from your ear.",
      "Oomph!  You squeeze water out of a granite boulder.",
      "Oomph!  $n squeezes water out of a granite boulder."}
     },

    {
     {
      "Your head disappears.",
      "$n's head disappears.",
      "A cool breeze refreshes you.",
      "A cool breeze refreshes $n.",
      "You step behind your shadow.",
      "$n steps behind $s shadow.",
      "You pick your teeth with a spear.",
      "$n picks $s teeth with a spear."}
     },

    {
     {
      "A fire elemental singes your hair.",
      "A fire elemental singes $n's hair.",
      "The sun pierces through the clouds to illuminate you.",
      "The sun pierces through the clouds to illuminate $n.",
      "Your eyes dance with greed.",
      "$n's eyes dance with greed.",
      "Everyone is swept off their foot by your hug.",
      "You are swept off your feet by $n's hug."}
     },

    {
     {
      "The sky changes color to match your eyes.",
      "The sky changes color to match $n's eyes.",
      "The ocean parts before you.",
      "The ocean parts before $n.",
      "You deftly steal everyone's weapon.",
      "$n deftly steals your weapon.",
      "Your karate chop splits a tree.",
      "$n's karate chop splits a tree."}
     },

    {
     {
      "The stones dance to your command.",
      "The stones dance to $n's command.",
      "A thunder cloud kneels to you.",
      "A thunder cloud kneels to $n.",
      "The Grey Mouser buys you a beer.",
      "The Grey Mouser buys $n a beer.",
      "A strap of your armor breaks over your mighty thews.",
      "A strap of $n's armor breaks over $s mighty thews."}
     },

    {
     {
      "The heavens and grass change colour as you smile.",
      "The heavens and grass change colour as $n smiles.",
      "The Burning Man speaks to you.",
      "The Burning Man speaks to $n.",
      "Everyone's pocket explodes with your fireworks.",
      "Your pocket explodes with $n's fireworks.",
      "A boulder cracks at your frown.",
      "A boulder cracks at $n's frown."}
     },

    {
     {
      "Everyone's clothes are transparent, and you are laughing.",
      "Your clothes are transparent, and $n is laughing.",
      "An eye in a pyramid winks at you.",
      "An eye in a pyramid winks at $n.",
      "Everyone discovers your dagger a centimeter from their eye.",
      "You discover $n's dagger a centimeter from your eye.",
      "Mercenaries arrive to do your bidding.",
      "Mercenaries arrive to do $n's bidding."}
     },

    {
     {
      "A black hole swallows you.",
      "A black hole swallows $n.",
      "Valentine Michael Smith offers you a glass of water.",
      "Valentine Michael Smith offers $n a glass of water.",
      "Where did you go?",
      "Where did $n go?",
      "Four matched Percherons bring in your chariot.",
      "Four matched Percherons bring in $n's chariot."}
     },

    {
     {
      "The world shimmers in time with your whistling.",
      "The world shimmers in time with $n's whistling.",
      "The Creator gives you a staff.",
      "The Creator gives $n a staff.",
      "Click.",
      "Click.",
      "Atlas asks you to relieve him.",
      "Atlas asks $n to relieve him."}
     }
};



void do_pose (CHAR_DATA * ch, char *argument)
{
    int level;
    int pose;

    if (IS_NPC (ch))
        return;

    level =
        UMIN (ch->level, int(sizeof (pose_table) / sizeof (pose_table[0])) - 1);
    pose = number_range (0, level);

    act (pose_table[pose].message[2 * ch->cClass + 0], ch, NULL, NULL,
         TO_CHAR);
    act (pose_table[pose].message[2 * ch->cClass + 1], ch, NULL, NULL,
         TO_ROOM);

    return;
}

void do_rent (CHAR_DATA * ch, char *argument)
{
    send_to_char ("There is no rent here.  Just save and quit.\n\r", ch);
    return;
}


void do_qui (CHAR_DATA * ch, char *argument)
{
    send_to_char ("If you want to QUIT, you have to spell it out.\n\r", ch);
    return;
}



void do_quit (CHAR_DATA * ch, char *argument)
{
    DESCRIPTOR_DATA *d, *d_next;
    int id;

    if (IS_NPC (ch))
        return;

    if (ch->position == POS_FIGHTING)
    {
        send_to_char ("No way! You are fighting.\n\r", ch);
        return;
    }

    if (ch->pcdata->pk_timer > 0)
    {
      send_to_char("Can't get away that easy.\n\r", ch);
      return;
    }


    if IS_SET(ch->act, PLR_TOURNEY)
    {
     REMOVE_BIT(ch->act, PLR_TOURNEY);
    }
    if (IS_SET(ch->act, PLR_TAG))
    {
     REMOVE_BIT(ch->act, PLR_TAG);
    } 
    if (IS_SET(ch->act, PLR_QUESTOR))
    {
     REMOVE_BIT(ch->act, PLR_QUESTOR);
    }
    if (IS_SET(ch->act, PLR_QUESTING))
    {
     REMOVE_BIT(ch->act, PLR_QUESTING);
    }
    if (IS_SET(ch->act, PLR_IT))
    {
     REMOVE_BIT(ch->act, PLR_IT);
    }
    if (IS_SET(ch->act2, PLR_TOURNAMENT_START))
    {
     REMOVE_BIT(ch->act2, PLR_TOURNAMENT_START);
     tournament.players -= 1;
    }

    if (ch->position < POS_STUNNED)
    {
        send_to_char ("You're not DEAD yet.\n\r", ch);
        return;
    }

    if ( auction_info.high_bidder == ch || auction_info.owner == ch )
    {
        send_to_char("You still have a stake in the auction!\n\r",ch);
        return;
    }

    abort_quest(ch);
    if (IS_DISGUISED (ch))
        REM_DISGUISE(ch);
    send_to_char ("Alas, all good things must come to an end.\n\r", ch);
    act ("$n has left the game.", ch, NULL, NULL, TO_ROOM);
    sprintf (log_buf, "%s has quit.", ch->name);
    log_string (log_buf);
    wiznet ("$N rejoins the real world.", ch, NULL, WIZ_LOGINS, 0,
            get_trust (ch));

    /*
     * After extract_char the ch is no longer valid!
     */
    save_char_obj (ch);
    id = ch->id;
    d = ch->desc;
    extract_char (ch, TRUE);

    if (d != NULL && d->account)
    {
      show_account(d, "");
      return;
    }

    if (d != NULL)
        close_socket (d);

    /* toast evil cheating bastards */
    for (d = descriptor_list; d != NULL; d = d_next)
    {
        CHAR_DATA *tch;

        d_next = d->next;
        tch = d->original ? d->original : d->character;
        if (tch && tch->id == id)
        {
            extract_char (tch, TRUE);
            close_socket (d);
        }
    }

    return;
}



void do_save (CHAR_DATA * ch, char *argument)
{
    if (IS_NPC (ch))
        return;


    save_char_obj (ch);
    send_to_char ("Saving...\n\r",
                  ch);
    return;
}



void do_follow (CHAR_DATA * ch, char *argument)
{
/* RT changed to allow unlimited following and follow the NOFOLLOW rules */
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        send_to_char ("Follow whom?\n\r", ch);
        return;
    }

    if ((victim = get_char_room (ch, NULL, arg)) == NULL)
    {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }

    if (IS_AFFECTED (ch, AFF_CHARM) && ch->master != NULL)
    {
        act ("But you'd rather follow $N!", ch, NULL, ch->master, TO_CHAR);
        return;
    }

    if (victim == ch)
    {
        if (ch->master == NULL)
        {
            send_to_char ("You already follow yourself.\n\r", ch);
            return;
        }

        ch->master = NULL;
        send_to_char("You will lead your own way from now on.\n\r", ch);
        return;
    }

    if (!IS_NPC (victim) && IS_SET (victim->act, PLR_NOFOLLOW)
        && !IS_IMMORTAL (ch))
    {
        act ("$N doesn't seem to want any followers.\n\r", ch, NULL, victim,
             TO_CHAR);
        return;
    }

    REMOVE_BIT (ch->act, PLR_NOFOLLOW);

    if (ch->master == NULL)
    {
      ch->master = victim;
      if (can_see (victim, ch))
        act ("$n now follows you.", ch, NULL, victim, TO_VICT);

      act ("You now follow $N.", ch, NULL, victim, TO_CHAR);
      return;
    } 

    if (ch->master != NULL)
        stop_follower (ch);

    add_follower (ch, victim);
    return;
}


void add_follower (CHAR_DATA * ch, CHAR_DATA * master)
{
    if (ch->master != NULL)
    {
        bug ("Add_follower: non-null master.", 0);
        return;
    }

    ch->master = master;
    ch->leader = NULL;

    if (can_see (master, ch))
        act ("$n now follows you.", ch, NULL, master, TO_VICT);

    act ("You now follow $N.", ch, NULL, master, TO_CHAR);

    return;
}



void stop_follower (CHAR_DATA * ch)
{
    if (ch->master == NULL)
    {
        bug ("Stop_follower: null master.", 0);
        return;
    }

    if (IS_AFFECTED (ch, AFF_CHARM))
    {
        STR_REMOVE_BIT (ch->affected_by, AFF_CHARM);
        affect_strip (ch, gsn_charm_person);
    }

    if (can_see (ch->master, ch) && ch->in_room != NULL)
    {
        act ("$n stops following you.", ch, NULL, ch->master, TO_VICT);
        act ("You stop following $N.", ch, NULL, ch->master, TO_CHAR);
    }
    if (ch->master->pet == ch)
        ch->master->pet = NULL;

    ch->master = NULL;
    ch->leader = NULL;
    return;
}


void die_mount (CHAR_DATA *ch)
{ 
    CHAR_DATA *fch;

    ch->mount = NULL;
    ch->is_mounted = FALSE;

    for (fch = char_list; fch != NULL; fch = fch->next)
    {
        if (fch->mount == ch)
	{
          fch->mount = NULL;
	  fch->is_mounted = FALSE;
	}
    }

    return;
}

void die_follower (CHAR_DATA * ch)
{
    CHAR_DATA *fch;

    if (ch->master != NULL)
    {
        if (ch->master->pet == ch)
            ch->master->pet = NULL;
        stop_follower (ch);
    }

    ch->leader = NULL;

    for (fch = char_list; fch != NULL; fch = fch->next)
    {
        if (fch->master == ch)
            stop_follower (fch);
        if (fch->leader == ch)
            fch->leader = fch;
    }

    return;
}



void do_order (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *och;
    CHAR_DATA *och_next;
    bool found;
    bool fAll;

    argument = one_argument (argument, arg);
    one_argument (argument, arg2);

    if (!str_cmp (arg2, "delete") || !str_cmp (arg2, "mob"))
    {
        send_to_char ("That will NOT be done.\n\r", ch);
        return;
    }

    if (arg[0] == '\0' || argument[0] == '\0')
    {
        send_to_char ("Order whom to do what?\n\r", ch);
        return;
    }

    if (IS_AFFECTED (ch, AFF_CHARM))
    {
        send_to_char ("You feel like taking, not giving, orders.\n\r", ch);
        return;
    }

    if (!str_cmp (arg, "all"))
    {
        fAll = TRUE;
        victim = NULL;
    }
    else
    {
        fAll = FALSE;
        if ((victim = get_char_room (ch, NULL, arg)) == NULL)
        {
            send_to_char ("They aren't here.\n\r", ch);
            return;
        }

        if (victim == ch)
        {
            send_to_char ("Aye aye, right away!\n\r", ch);
            return;
        }

        if (!IS_AFFECTED (victim, AFF_CHARM) || victim->master != ch
            || (IS_IMMORTAL (victim) && victim->trust >= ch->trust))
        {
            send_to_char ("Do it yourself!\n\r", ch);
            return;
        }
    }

    found = FALSE;
    for (och = ch->in_room->people; och != NULL; och = och_next)
    {
        och_next = och->next_in_room;

        if (IS_AFFECTED (och, AFF_CHARM)
            && och->master == ch && (fAll || och == victim))
        {
            found = TRUE;
            sprintf (buf, "$n orders you to '%s'.", argument);
            act (buf, ch, NULL, och, TO_VICT);
            interpret (och, argument);
        }
    }

    if (found)
    {
        WAIT_STATE (ch, PULSE_VIOLENCE);
        send_to_char ("Ok.\n\r", ch);
    }
    else
        send_to_char ("You have no followers here.\n\r", ch);
    return;
}

int count_charmed(CHAR_DATA *ch)
{
  CHAR_DATA *gch;
  int i = 0;

  for (gch = char_list; gch != NULL; gch = gch->next)
  {
    if (IS_NPC(gch) && gch->master == ch)
    {
      i++;
    }
  }
  return i;
}


void do_group (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument (argument, arg);

    if (arg[0] == '\0')
    {
        CHAR_DATA *gch;
        CHAR_DATA *leader;

        leader = (ch->leader != NULL) ? ch->leader : ch;
        sprintf (buf, "%s's group:\n\r", PERS (leader, ch, FALSE));
        send_to_char (buf, ch);

        for (gch = char_list; gch != NULL; gch = gch->next)
        {
            if (is_same_group (gch, ch))
            {
                sprintf (buf,
                         "[%2d %s] %-16s %4ld/%4ld hp %4d/%4d mana %4d/%4d mv %5d xp\n\r",
                         gch->level,
                         IS_NPC (gch) ? "Mob" : class_table[gch->cClass].who_name,
                         capitalize (PERS (gch, ch, FALSE)), gch->hit, gch->max_hit,
                         gch->mana, gch->max_mana, gch->move, gch->max_move,
                         gch->exp);
                send_to_char (buf, ch);
            }
        }
        return;
    }

    if ((victim = get_char_room (ch, NULL, arg)) == NULL)
    {
        send_to_char ("They aren't here.\n\r", ch);
        return;
    }

    if (ch->master != NULL || (ch->leader != NULL && ch->leader != ch))
    {
        send_to_char ("But you are following someone else!\n\r", ch);
        return;
    }

    if (victim->master != ch && ch != victim)
    {
        act_new ("$N isn't following you.", ch, NULL, victim, TO_CHAR,
                 POS_SLEEPING);
        return;
    }

    if (IS_AFFECTED (victim, AFF_CHARM))
    {
        send_to_char ("You can't remove charmed mobs from your group.\n\r",
                      ch);
        return;
    }

    if (IS_AFFECTED (ch, AFF_CHARM))
    {
        act_new ("You like your master too much to leave $m!",
                 ch, NULL, victim, TO_VICT, POS_SLEEPING);
        return;
    }

    if (is_same_group (victim, ch) && ch != victim)
    {
        victim->leader = NULL;
        act_new ("$n removes $N from $s group.",
                 ch, NULL, victim, TO_NOTVICT, POS_RESTING);
        act_new ("$n removes you from $s group.",
                 ch, NULL, victim, TO_VICT, POS_SLEEPING);
        act_new ("You remove $N from your group.",
                 ch, NULL, victim, TO_CHAR, POS_SLEEPING);
        return;
    }

    victim->leader = ch;
    act_new ("$N joins $n's group.", ch, NULL, victim, TO_NOTVICT,
             POS_RESTING);
    act_new ("You join $n's group.", ch, NULL, victim, TO_VICT, POS_SLEEPING);
    act_new ("$N joins your group.", ch, NULL, victim, TO_CHAR, POS_SLEEPING);
    return;
}



/*
 * 'Split' originally by Gnort, God of Chaos.
 */
void do_split (CHAR_DATA * ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *gch;
    int members;
    int amount_gold = 0, amount_silver = 0;
    int share_gold, share_silver;
    int extra_gold, extra_silver;

    argument = one_argument (argument, arg1);
    one_argument (argument, arg2);

    if (arg1[0] == '\0')
    {
        send_to_char ("Split how much?\n\r", ch);
        return;
    }

    amount_silver = atoi (arg1);

    if (arg2[0] != '\0')
        amount_gold = atoi (arg2);

    if (amount_gold < 0 || amount_silver < 0)
    {
        send_to_char ("Your group wouldn't like that.\n\r", ch);
        return;
    }

    if (amount_gold == 0 && amount_silver == 0)
    {
        send_to_char ("You hand out zero coins, but no one notices.\n\r", ch);
        return;
    }

    if (ch->gold < amount_gold || ch->silver < amount_silver)
    {
        send_to_char ("You don't have that much to split.\n\r", ch);
        return;
    }

    members = 0;
    for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
    {
        if (is_same_group (gch, ch) && !IS_AFFECTED (gch, AFF_CHARM))
            members++;
    }

    if (members < 2)
    {
        send_to_char ("Just keep it all.\n\r", ch);
        return;
    }

    share_silver = amount_silver / members;
    extra_silver = amount_silver % members;

    share_gold = amount_gold / members;
    extra_gold = amount_gold % members;

    if (share_gold == 0 && share_silver == 0)
    {
        send_to_char ("Don't even bother, cheapskate.\n\r", ch);
        return;
    }

    ch->silver -= amount_silver;
    ch->silver += share_silver + extra_silver;
    ch->gold -= amount_gold;
    ch->gold += share_gold + extra_gold;

    if (share_silver > 0)
    {
        sprintf (buf,
                 "You split %d silver coins. Your share is %d silver.\n\r",
                 amount_silver, share_silver + extra_silver);
        send_to_char (buf, ch);
    }

    if (share_gold > 0)
    {
        sprintf (buf,
                 "You split %d gold coins. Your share is %d gold.\n\r",
                 amount_gold, share_gold + extra_gold);
        send_to_char (buf, ch);
    }

    if (share_gold == 0)
    {
        sprintf (buf, "$n splits %d silver coins. Your share is %d silver.",
                 amount_silver, share_silver);
    }
    else if (share_silver == 0)
    {
        sprintf (buf, "$n splits %d gold coins. Your share is %d gold.",
                 amount_gold, share_gold);
    }
    else
    {
        sprintf (buf,
                 "$n splits %d silver and %d gold coins, giving you %d silver and %d gold.\n\r",
                 amount_silver, amount_gold, share_silver, share_gold);
    }

    for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room)
    {
        if (gch != ch && is_same_group (gch, ch)
            && !IS_AFFECTED (gch, AFF_CHARM))
        {
            act (buf, ch, NULL, gch, TO_VICT);
            gch->gold += share_gold;
            gch->silver += share_silver;
        }
    }

    return;
}



void do_gtell (CHAR_DATA * ch, char *argument)
{
    CHAR_DATA *gch;

    if (argument[0] == '\0')
    {
        send_to_char ("Tell your group what?\n\r", ch);
        return;
    }

    if (IS_SET (ch->comm, COMM_NOTELL))
    {
        send_to_char ("Your message didn't get through!\n\r", ch);
        return;
    }

    act_new("You tell the group '`@$t`*'", 
               ch, argument, NULL, TO_CHAR, POS_SLEEPING);
    for (gch = char_list; gch != NULL; gch = gch->next)
    {
        if (is_same_group (gch, ch))
            act_new ("$n tells the group '`@$t`*'",
                     ch, argument, gch, TO_VICT, POS_SLEEPING);
    }

    return;
}



/*
 * It is very important that this be an equivalence relation:
 * (1) A ~ A
 * (2) if A ~ B then B ~ A
 * (3) if A ~ B  and B ~ C, then A ~ C
 */
bool is_same_group (CHAR_DATA * ach, CHAR_DATA * bch)
{
    if (ach == NULL || bch == NULL)
        return FALSE;

    if (ach->leader != NULL)
        ach = ach->leader;
    if (bch->leader != NULL)
        bch = bch->leader;
    return ach == bch;
}

/*
 * ColoUr setting and unsetting, way cool, Ant Oct 94
 *        revised to include config colour, Ant Feb 95
 */
void do_colour (CHAR_DATA * ch, char *argument)
{
    char arg[MAX_STRING_LENGTH];

    if (IS_NPC (ch))
    {
        send_to_char_bw ("ColoUr is not ON, Way Moron!\n\r", ch);
        return;
    }

    argument = one_argument (argument, arg);

    if (!*arg)
    {
        if (!IS_SET (ch->act, PLR_COLOUR))
        {
            SET_BIT (ch->act, PLR_COLOUR);
            send_to_char ("ColoUr is now ON, Way Cool!\n\r"
                          "Further syntax:\n\r   colour `6<`*field`6> <`*colour`6>`*\n\r"
                          "   colour `6<`*field`6>`* `6beep`*|`6nobeep`*\n\r"
                          "Type help `6colour`* and `6colour2`* for details.\n\r"
                          "ColoUr is brought to you by Lope, ant@solace.mh.se.\n\r",
                          ch);
        }
        else
        {
            send_to_char_bw ("ColoUr is now OFF, <sigh>\n\r", ch);
            REMOVE_BIT (ch->act, PLR_COLOUR);
        }
        return;
    }

    if (!str_cmp (arg, "default"))
    {
        default_colour (ch);
        send_to_char_bw ("ColoUr setting set to default values.\n\r", ch);
        return;
    }

    if (!str_cmp (arg, "all"))
    {
        all_colour (ch, argument);
        return;
    }

    return;
}
