//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#if !defined( PARSEMSG_H )
#define PARSEMSG_H
#ifdef _WIN32
#pragma once
#endif

class QAngle;
class Vector;

void	BEGIN_READ( void *buf, int size );

// bytewise
int		READ_CHAR( void );
int		READ_BYTE( void );
int		READ_SHORT( void );
int		READ_WORD( void );
int		READ_LONG( void );
float	READ_FLOAT( void );
char	*READ_STRING( void );
float	READ_COORD( void );
float	READ_ANGLE( void );
void	READ_VEC3COORD( Vector& vec );
void	READ_VEC3NORMAL( Vector& vec );
float	READ_HIRESANGLE( void );
void	READ_ANGLES( QAngle &angles );

// bitwise
bool	READ_BOOL( void );
unsigned int READ_UBITLONG( int numbits );
int		READ_SBITLONG( int numbits );
void	READ_BITS( void *buffer, int nbits );

#endif // PARSEMSG_H