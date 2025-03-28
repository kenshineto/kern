/**
** @file	userids.h
**
** @author	Warren R. Carithers
**
** @brief	IDs for user-level programs
**
** NOTE: this file is automatically generated when the user.img file
** is created. Do not edit this manually!
*/

#ifndef USERIDS_H_
#define USERIDS_H_

#ifndef ASM_SRC
/*
** These IDs are used to identify the various user programs.
** Each call to exec() will provide one of these as the first
** argument.
**
** This list should be updated if/when the collection of
** user processes changes.
*/
enum users_e {
	Init,
	Idle,
	Shell,
	ProgABC,
	ProgDE,
	ProgFG,
	ProgH,
	ProgI,
	ProgJ,
	ProgKL,
	ProgMN,
	ProgP,
	ProgQ,
	ProgR,
	ProgS,
	ProgTUV,
	ProgW,
	ProgX,
	ProgY,
	ProgZ
	// sentinel
	,
	N_USERS
};
#endif /* !ASM_SRC */

#endif
