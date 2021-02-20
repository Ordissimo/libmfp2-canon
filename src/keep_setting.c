/*
 *  ScanGear MP for Linux
 *  Copyright CANON INC. 2007-2020
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA.
 *
 * NOTE:
 *  - As a special exception, this program is permissible to link with the
 *    libraries released as the binary modules.
 *  - If you write modifications of your own for these programs, it is your
 *    choice whether to permit this exception to apply to your modifications.
 *    If you do not wish that, delete this exception.
*/

#ifndef	_KEEP_SETTING_C_
#define	_KEEP_SETTING_C_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>
#include <limits.h>

#include "support.h"

#include "cnmstype.h"
#include "cnmsfunc.h"
#include "file_control.h"
#include "keep_setting.h"

/*	#define	__CNMS_DEBUG_KEEP_SETTING__	*/

#define	KEEP_SETTING_RASTER_LEN		(256)
#define	KEEP_SETTING_HEADER_STR		("/***** Do not edit this file. *****/\n")

static CNMSLPSTR	KeepSettingCommonStrArray[ KEEPSETTING_COMMON_ID_MAX ]
					= { "MAC Address:\0" };

typedef struct{
	CNMSByte		file_path[ PATH_MAX ];
	CNMSByte		str[ KEEPSETTING_COMMON_ID_MAX ][ PATH_MAX ];
}KEEPSETTINGCOMMONCOMP, *LPKEEPSETTINGCOMMONCOMP;

static LPKEEPSETTINGCOMMONCOMP	lpCommonSetting = CNMSNULL;

static CNMSInt32 SubReadSettingFile( CNMSFd fd );
static CNMSInt32 SubWriteSettingFile( CNMSFd fd );
#ifdef	__CNMS_DEBUG_KEEP_SETTING__
static CNMSVoid DebugKeepSettingComp( CNMSVoid );
#endif

static CNMSInt32 SubWriteSettingCommonFile( CNMSFd fd );
static CNMSInt32 SubReadSettingCommonFile( CNMSFd fd );


CNMSInt32 KeepSettingCommonOpen( CNMSVoid )
{
	CNMSInt32	ret = CNMS_ERR, i, ldata;
	CNMSFd		keepFd = CNMS_FILE_ERR;

	/* check */
	if( lpCommonSetting != CNMSNULL ){
		KeepSettingCommonClose();
	}
	if( ( lpCommonSetting = (LPKEEPSETTINGCOMMONCOMP)CnmsGetMem( sizeof( KEEPSETTINGCOMMONCOMP ) ) ) == CNMSNULL ){
		DBGMSG( "[KeepSettingCommonOpen]Error is occured in CnmsGetMem.\n" );
		goto	EXIT;
	}
	
	/* init */
	snprintf( lpCommonSetting->file_path, PATH_MAX, "/var/tmp/canon_sgmp2_setting.ini" );	/* Ver.2.20 */
	if( ( ldata = CnmsStrLen( lpCommonSetting->file_path ) ) <= 0 ){
		DBGMSG( "[KeepSettingCommonOpen]Error is occured in CnmsStrLen.\n" );
		goto	EXIT;
	}
	for( i = 0 ; i < ldata ; i ++ ){
		if( lpCommonSetting->file_path[ i ] == ' ' ){
			DBGMSG( "[KeepSettingCommonOpen]Error is occured in CnmsStrLen.\n" );
			goto	EXIT;
		}
	}
	/* set default value */
	ldata = FileControlGetStatus( lpCommonSetting->file_path, PATH_MAX );
	if( ldata == FILECONTROL_STATUS_NOT_EXIST ){
		if( KeepSettingCommonSetString( KEEPSETTING_COMMON_ID_MACADDRESS, KEEPSETTING_MAC_ADDRESS_USB ) != CNMS_NO_ERR ){
			DBGMSG( "[KeepSettingCommonOpen]Error is occured in KeepSettingCommonSetString.\n" );
			goto	EXIT;
		}
	}
	else if( ( ldata == FILECONTROL_STATUS_WRITE_OK ) || ( ldata == FILECONTROL_STATUS_WRITE_NG ) ){
		/* read setting from file */
		if( ( keepFd = FileControlOpenFile( FILECONTROL_OPEN_TYPE_READ, lpCommonSetting->file_path ) ) == CNMS_FILE_ERR ){
			DBGMSG( "[KeepSettingCommonOpen]Error is occured in FileControlOpenFile.\n" );
			goto	EXIT;
		}
		else if( ( ldata = SubReadSettingCommonFile( keepFd ) ) != CNMS_NO_ERR ){
			DBGMSG( "[KeepSettingCommonOpen]Error is occured in SubReadSettingCommonFile.\n" );
			goto	EXIT;
		}
	}

	ret = CNMS_NO_ERR;
EXIT:
	if( keepFd != CNMS_FILE_ERR ){
		FileControlCloseFile( keepFd );
	}
	if( ret != CNMS_NO_ERR ){
		KeepSettingCommonClose();
	}
#ifdef	__CNMS_DEBUG_KEEP_SETTING__
	DBGMSG( "[KeepSettingCommonOpen()]=%d.\n", ret );
#endif
	return	ret;
}

static CNMSInt32 SubReadSettingCommonFile( CNMSFd fd )
{
	CNMSInt32	ret = CNMS_ERR, rasSize, i, j, ldata;
	CNMSLPSTR	lpBuf = CNMSNULL, lpStr;

	if( ( lpBuf = CnmsGetMem( KEEP_SETTING_RASTER_LEN ) ) == CNMSNULL ){
		goto	EXIT;
	}
	while( 1 ){
		if( ( rasSize = FileControlReadRasterString( fd, lpBuf, KEEP_SETTING_RASTER_LEN ) ) == CNMS_ERR ){
			DBGMSG( "[SubReadSettingFile]Error is occured in FileControlReadRasterString.\n" );
			goto	EXIT;
		}
		else if( rasSize == 0 ){
			break;	/* file end */
		}
		for( i = 0 ; i < KEEPSETTING_COMMON_ID_MAX ; i ++ ){
			lpStr = KeepSettingCommonStrArray[ i ];
			ldata = CNMS_ERR;
			for( j = 0 ; j < rasSize ; j ++ ){
				if( lpBuf[ j ] != lpStr[ j ] ){
					if( lpStr[ j ] == '\0' ){
						CnmsStrCopy( &(lpBuf[ j ]), lpCommonSetting->str[ i ], PATH_MAX );
					}
					break;
				}
			}
		}
	}
	ret = CNMS_NO_ERR;
EXIT:
	if( lpBuf != CNMSNULL ){
		CnmsFreeMem( lpBuf );
	}
	return	ret;
}

CNMSVoid KeepSettingCommonClose( CNMSVoid )
{
	if( lpCommonSetting != CNMSNULL ){
		CnmsFreeMem( (CNMSLPSTR)lpCommonSetting );
	}
	lpCommonSetting = CNMSNULL;

#ifdef	__CNMS_DEBUG_KEEP_SETTING__
	DBGMSG( "[KeepSettingCommonClose()].\n" );
#endif
	return;
}

CNMSLPSTR KeepSettingCommonGetString(
		CNMSInt32		id )
{
	CNMSLPSTR		ret = CNMSNULL;

	if( ( id < 0 ) || ( KEEPSETTING_COMMON_ID_MAX <= id ) || ( lpCommonSetting == CNMSNULL ) ){
		DBGMSG( "[KeepSettingCommonGetString]Parameter is error.\n" );
		goto	EXIT;
	}

	ret = lpCommonSetting->str[ id ];
EXIT:
#ifdef	__CNMS_DEBUG_KEEP_SETTING__
	DBGMSG( "[KeepSettingCommonGetString(id:%d)]=%s.\n", id, ret );
#endif
	return	ret;
}

CNMSInt32 KeepSettingCommonSetString(
		CNMSInt32		id,
		CNMSLPSTR		str )
{
	CNMSInt32		ret = CNMS_ERR, ldata;
	CNMSFd			keepFd = CNMS_FILE_ERR;

	if( ( id < 0 ) || ( KEEPSETTING_COMMON_ID_MAX <= id ) || ( lpCommonSetting == CNMSNULL ) ){
		DBGMSG( "[KeepSettingCommonSetString]Parameter is error.\n" );
		goto	EXIT;
	}
	if( CnmsStrCopy( str, lpCommonSetting->str[id], CnmsStrLen(str)+1 ) == CNMS_ERR ){
		DBGMSG( "[KeepSettingCommonSetString]Parameter is error.\n" );
		goto	EXIT;
	}
		
	if( ( keepFd = FileControlOpenFile( FILECONTROL_OPEN_TYPE_NEW_ALL, lpCommonSetting->file_path ) ) == CNMS_FILE_ERR ){
		goto	EXIT;
	}
	if( ( ldata = SubWriteSettingCommonFile( keepFd ) ) != CNMS_NO_ERR ){
		goto	EXIT;
	}

	ret = CNMS_NO_ERR;
EXIT:
	if( keepFd != CNMS_FILE_ERR ){
		FileControlCloseFile( keepFd );
	}
#ifdef	__CNMS_DEBUG_KEEP_SETTING__
	DBGMSG( "[KeepSettingCommonSetString(str:%s)]=%d.\n", str, ret );
#endif
	return	ret;
}

static CNMSInt32 SubWriteSettingCommonFile( CNMSFd fd )
{
	CNMSInt32	ret = CNMS_ERR, i, ldata;
	CNMSLPSTR	lpBuf = CNMSNULL;

	if( ( lpBuf = CnmsGetMem( KEEP_SETTING_RASTER_LEN ) ) == CNMSNULL ){
		goto	EXIT;
	}
	/* write header */
	if( ( ldata = FileControlWriteFile( fd, KEEP_SETTING_HEADER_STR, CnmsStrLen( KEEP_SETTING_HEADER_STR ) ) ) != CNMS_NO_ERR ){
		goto	EXIT;
	}
	for( i = 0 ; i < KEEPSETTING_COMMON_ID_MAX ; i ++ ){
		snprintf( lpBuf, KEEP_SETTING_RASTER_LEN, "%s%s\n", KeepSettingCommonStrArray[ i ], lpCommonSetting->str[ i ] );
		if( ( ldata = FileControlWriteFile( fd, lpBuf, CnmsStrLen( lpBuf ) ) ) != CNMS_NO_ERR ){
			goto	EXIT;
		}
	}
	ret = CNMS_NO_ERR;
EXIT:
	if( lpBuf != CNMSNULL ){
		CnmsFreeMem( lpBuf );
	}
	return	ret;
}



#ifdef	__CNMS_DEBUG_KEEP_SETTING__
static CNMSVoid DebugKeepSettingComp( CNMSVoid )
{
	CNMSInt32	i;

	if( lpSetting == CNMSNULL ){
		DBGMSG( "[DebugKeepSettingComp]No initialized.\n" );
		goto	EXIT;
	}
	for( i = 0 ; i < KEEPSETTING_ID_MAX ; i ++ ){
		DBGMSG( "[DebugKeepSettingComp]No.%2d:%d\n", i, lpSetting->value[ i ] );
	}
EXIT:
	return;
}
#endif

#endif	/* _KEEP_SETTING_C_ */

