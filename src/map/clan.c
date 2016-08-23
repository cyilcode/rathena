// Copyright (c) Athena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "../common/cbasetypes.h"
#include "../common/mmo.h"
#include "../common/malloc.h"
#include "../common/nullpo.h"
#include "../common/showmsg.h"

#include "clan.h"
#include "clif.h"
#include "intif.h"
#include "pc.h" 
#include "script.h"
#include "status.h"

static DBMap* clan_db; // int clan_id -> struct clan*

void do_init_clan(){
	clan_db = idb_alloc(DB_OPT_RELEASE_DATA);
}

void do_final_clan(){
	db_destroy(clan_db);
}

void clan_load_clandata( int count, struct clan* clans ){
	int i,j;

	nullpo_retv( clans );

	for( i = 0, j = 0; i < count; i++, clans++ ){
		struct clan* clan = clans;
		struct clan* clanCopy;

		clanCopy = (struct clan*)aMalloc( sizeof( struct clan ) );

		if( clanCopy == NULL ){
			ShowError("Memory could not be allocated for a clan.\n");
			break;
		}

		memcpy( clanCopy, clan, sizeof( struct clan ) );
		memset( clanCopy->members, 0, sizeof( clanCopy->members ) );

		idb_put( clan_db, clanCopy->id, clanCopy );
		j++;
	}

	ShowStatus( "Received '"CL_WHITE"%d"CL_RESET"' clans from char-server.\n", j );
}

struct clan* clan_search( int id ){
	return (struct clan*)idb_get(clan_db,id);
}

struct map_session_data* clan_getavailablesd( struct clan* clan ){
	int i;

	nullpo_retr(NULL, clan);

	ARR_FIND( 0, clan->max_member, i, clan->members[i] != NULL );
	return ( i < clan->max_member ) ? clan->members[i] : NULL;
}

int clan_getMemberIndex( struct clan* clan, uint32 account_id ){
	int i;

	nullpo_retr(-1,clan);

	ARR_FIND( 0, clan->max_member, i, clan->members[i] != NULL && clan->members[i]->status.account_id == account_id );

	if( i == clan->max_member ){
		return -1;
	}else{
		return i;
	}
}

int clan_getNextFreeMemberIndex( struct clan* clan ){
	int i;

	nullpo_retr(-1,clan);

	ARR_FIND( 0, clan->max_member, i, clan->members[i] == NULL );

	if( i == clan->max_member ){
		return -1;
	}else{
		return i;
	}
}

void clan_member_joined( struct map_session_data* sd ){
	struct clan* clan;
	int index;

	nullpo_retv(sd);

	clan = clan_search(sd->status.clan_id);

	nullpo_retv(clan);
	
	if( ( index = clan_getNextFreeMemberIndex(clan) ) >= 0 ){
		sd->clan = clan;
		clan->members[index] = sd;
		clan->connect_member++;

		clif_clan_basicinfo(sd);

		intif_clan_member_joined(clan->id);
		clif_clan_onlinecount(clan);
	}
}

void clan_member_left( struct map_session_data* sd ){
	int index;
	struct clan* clan;

	nullpo_retv(sd);
	nullpo_retv(clan = sd->clan);

	if( ( index = clan_getMemberIndex(clan,sd->status.account_id) ) >= 0 ){
		clan->members[index] = NULL;
		clan->connect_member--;

		intif_clan_member_left(clan->id);
		clif_clan_onlinecount(clan);
	}
}

int clan_member_join( struct map_session_data *sd, int clan_id, uint32 account_id, uint32 char_id ){
	struct clan *clan;

	nullpo_ret(sd);
	
	if( ( clan = clan_search( clan_id ) ) == NULL ){
		return 0;
	}

	if( sd->status.account_id != account_id || sd->status.char_id != char_id || sd->status.clan_id != 0 ){
		return 0;
	}

	sd->status.clan_id = clan->id;

	clan_member_joined(sd);

	return 1;
}

int clan_member_leave( struct map_session_data* sd, int clan_id, uint32 account_id, uint32 char_id ){
	struct clan *clan;

	nullpo_ret(sd);

	if( sd->status.account_id != account_id || sd->status.char_id != char_id || sd->status.clan_id != clan_id || ( clan = sd->clan ) == NULL ){
		return 0;
	}

	clan_member_left(sd);

	sd->clan = NULL;
	sd->status.clan_id = 0;

	clif_clan_leave(sd);

	return 1;
}

void clan_recv_message(int clan_id,uint32 account_id,const char *mes,int len) {
	struct clan *clan;

	nullpo_retv( clan = clan_search(clan_id) );

	clif_clan_message(clan,mes,len);
}

void clan_send_message( struct map_session_data *sd, const char *mes, int len ){
	nullpo_retv(sd);
	nullpo_retv(sd->clan);

	intif_clan_message(sd->status.clan_id,sd->status.account_id,mes,len);
	clan_recv_message(sd->status.clan_id,sd->status.account_id,mes,len);
	log_chat( LOG_CHAT_CLAN, sd->status.clan_id, sd->status.char_id, sd->status.account_id, mapindex_id2name( sd->mapindex ), sd->bl.x, sd->bl.y, NULL, mes );
}
