/****************************/
/* THIS IS OPEN SOURCE CODE */
/****************************/

/** 
 * @file    linux-NWunit.c
 * @author  Heike Jagode
 *          jagode@eecs.utk.edu
 * Mods:	< your name here >
 *			< your email address >
 * BGPM / NWunit component 
 * 
 * Tested version of bgpm (early access)
 *
 * @brief
 *  This file has the source code for a component that enables PAPI-C to 
 *  access hardware monitoring counters for BG/Q through the bgpm library.
 */

#include "linux-NWunit.h"

/* Declare our vector in advance */
papi_vector_t _NWunit_vector;

/*****************************************************************************
 *******************  BEGIN PAPI's COMPONENT REQUIRED FUNCTIONS  *************
 *****************************************************************************/

/*
 * This is called whenever a thread is initialized
 */
int
NWUNIT_init_thread( hwd_context_t * ctx )
{
#ifdef DEBUG_BGQ
	printf( "NWUNIT_init_thread\n" );
#endif
	
	( void ) ctx;
	return PAPI_OK;
}


/* Initialize hardware counters, setup the function vector table
 * and get hardware information, this routine is called when the 
 * PAPI process is initialized (IE PAPI_library_init)
 */
int
NWUNIT_init_component( int cidx )
{  
#ifdef DEBUG_BGQ
	printf( "NWUNIT_init_component\n" );
#endif

	_NWunit_vector.cmp_info.CmpIdx = cidx;
#ifdef DEBUG_BGQ
	printf( "NWUNIT_init_component cidx = %d\n", cidx );
#endif
	
	return ( PAPI_OK );
}


/*
 * Control of counters (Reading/Writing/Starting/Stopping/Setup)
 * functions
 */
int
NWUNIT_init_control_state( hwd_control_state_t * ptr )
{
#ifdef DEBUG_BGQ
	printf( "NWUNIT_init_control_state\n" );
#endif
	int retval;

	NWUNIT_control_state_t * this_state = ( NWUNIT_control_state_t * ) ptr;
	
	this_state->EventGroup = Bgpm_CreateEventSet();
	retval = _check_BGPM_error( this_state->EventGroup, "Bgpm_CreateEventSet" );
	if ( retval < 0 ) return retval;

	return PAPI_OK;
}


/*
 *
 */
int
NWUNIT_start( hwd_context_t * ctx, hwd_control_state_t * ptr )
{
#ifdef DEBUG_BGQ
	printf( "NWUNIT_start\n" );
#endif
	
	( void ) ctx;
	int retval;
	NWUNIT_control_state_t * this_state = ( NWUNIT_control_state_t * ) ptr;

	retval = Bgpm_Attach( this_state->EventGroup, UPC_NW_ALL_LINKS, 0); 
	retval = _check_BGPM_error( retval, "Bgpm_Attach" );
	if ( retval < 0 ) return retval;

	retval = Bgpm_ResetStart( this_state->EventGroup );
	retval = _check_BGPM_error( retval, "Bgpm_ResetStart" );
	if ( retval < 0 ) return retval;

	return ( PAPI_OK );
}


/*
 *
 */
int
NWUNIT_stop( hwd_context_t * ctx, hwd_control_state_t * ptr )
{
#ifdef DEBUG_BGQ
	printf( "NWUNIT_stop\n" );
#endif
	( void ) ctx;
	int retval;
	NWUNIT_control_state_t * this_state = ( NWUNIT_control_state_t * ) ptr;
	
	retval = Bgpm_Stop( this_state->EventGroup );
	retval = _check_BGPM_error( retval, "Bgpm_Stop" );
	if ( retval < 0 ) return retval;

	return ( PAPI_OK );
}


/*
 *
 */
int
NWUNIT_read( hwd_context_t * ctx, hwd_control_state_t * ptr,
		   long_long ** events, int flags )
{
#ifdef DEBUG_BGQ
	printf( "NWUNIT_read\n" );
#endif
	( void ) ctx;
	( void ) flags;
	int i, numEvts;
	NWUNIT_control_state_t * this_state = ( NWUNIT_control_state_t * ) ptr;
	
	numEvts = Bgpm_NumEvents( this_state->EventGroup );
	if ( numEvts == 0 ) {
#ifdef DEBUG_BGPM
		printf ("Error: ret value is %d for BGPM API function Bgpm_NumEvents.\n", numEvts );
#endif
		//return ( EXIT_FAILURE );
	}
		
	for ( i = 0; i < numEvts; i++ )
		this_state->counts[i] = _common_getEventValue( i, this_state->EventGroup );

	*events = this_state->counts;
	
	return ( PAPI_OK );
}


/*
 *
 */
int
NWUNIT_shutdown_thread( hwd_context_t * ctx )
{
#ifdef DEBUG_BGQ
	printf( "NWUNIT_shutdown_thread\n" );
#endif
	
	( void ) ctx;
	return ( PAPI_OK );
}


/* This function sets various options in the component
 * The valid codes being passed in are PAPI_SET_DEFDOM,
 * PAPI_SET_DOMAIN, PAPI_SETDEFGRN, PAPI_SET_GRANUL * and PAPI_SET_INHERIT
 */
int
NWUNIT_ctl( hwd_context_t * ctx, int code, _papi_int_option_t * option )
{
#ifdef DEBUG_BGQ
	printf( "NWUNIT_ctl\n" );
#endif
	
	( void ) ctx;
	( void ) code;
	( void ) option;
	return ( PAPI_OK );
}


//int NWUNIT_ntv_code_to_bits ( unsigned int EventCode, hwd_register_t * bits );


/*
 *
 */
int
NWUNIT_update_control_state( hwd_control_state_t * ptr,
						   NativeInfo_t * native, int count,
						   hwd_context_t * ctx )
{
#ifdef DEBUG_BGQ
	printf( "NWUNIT_update_control_state: count = %d\n", count );
#endif
	( void ) ctx;
	int retval, index, i;
	NWUNIT_control_state_t * this_state = ( NWUNIT_control_state_t * ) ptr;
	
	// Delete and re-create BGPM eventset
	retval = _common_deleteRecreate( &this_state->EventGroup );
	if ( retval < 0 ) return retval;

	// otherwise, add the events to the eventset
	for ( i = 0; i < count; i++ ) {
		index = ( native[i].ni_event ) + OFFSET;
		
		native[i].ni_position = i;
		
#ifdef DEBUG_BGQ
		printf("NWUNIT_update_control_state: ADD event: i = %d, index = %d\n", i, index );
#endif
		
		/* Add events to the BGPM eventGroup */
		retval = Bgpm_AddEvent( this_state->EventGroup, index );
		retval = _check_BGPM_error( retval, "Bgpm_AddEvent" );
		if ( retval < 0 ) return retval; 
	}
	
	return ( PAPI_OK );
}


/*
 * This function has to set the bits needed to count different domains
 * In particular: PAPI_DOM_USER, PAPI_DOM_KERNEL PAPI_DOM_OTHER
 * By default return PAPI_EINVAL if none of those are specified
 * and PAPI_OK with success
 * PAPI_DOM_USER is only user context is counted
 * PAPI_DOM_KERNEL is only the Kernel/OS context is counted
 * PAPI_DOM_OTHER  is Exception/transient mode (like user TLB misses)
 * PAPI_DOM_ALL   is all of the domains
 */
int
NWUNIT_set_domain( hwd_control_state_t * cntrl, int domain )
{
#ifdef DEBUG_BGQ
	printf( "NWUNIT_set_domain\n" );
#endif
	int found = 0;
	( void ) cntrl;

	if ( PAPI_DOM_USER & domain )
		found = 1;

	if ( PAPI_DOM_KERNEL & domain )
		found = 1;

	if ( PAPI_DOM_OTHER & domain )
		found = 1;

	if ( !found )
		return ( PAPI_EINVAL );

	return ( PAPI_OK );
}


/*
 *
 */
int
NWUNIT_reset( hwd_context_t * ctx, hwd_control_state_t * ptr )
{
#ifdef DEBUG_BGQ
	printf( "NWUNIT_reset\n" );
#endif
	( void ) ctx;
	int retval;
	NWUNIT_control_state_t * this_state = ( NWUNIT_control_state_t * ) ptr;

	/* we can't simply call Bgpm_Reset() since PAPI doesn't have the 
	 restriction that an EventSet has to be stopped before resetting is
	 possible. However, BGPM does have this restriction. 
	 Hence we need to stop, reset and start */
	retval = Bgpm_Stop( this_state->EventGroup );
	retval = _check_BGPM_error( retval, "Bgpm_Stop" );
	if ( retval < 0 ) return retval;

	retval = Bgpm_ResetStart( this_state->EventGroup );
	retval = _check_BGPM_error( retval, "Bgpm_ResetStart" );
	if ( retval < 0 ) return retval;

	return ( PAPI_OK );
}


/*
 * PAPI Cleanup Eventset
 *
 * Destroy and re-create the BGPM / NWunit EventSet
 */
int
NWUNIT_cleanup_eventset( hwd_control_state_t * ctrl )
{
#ifdef DEBUG_BGQ
	printf( "NWUNIT_cleanup_eventset\n" );
#endif
	int retval;

	NWUNIT_control_state_t * this_state = ( NWUNIT_control_state_t * ) ctrl;
	
	// create a new empty bgpm eventset
	// reason: bgpm doesn't permit to remove events from an eventset; 
	// hence we delete the old eventset and create a new one
	retval = _common_deleteRecreate( &this_state->EventGroup ); // HJ try to use delete() only
	if ( retval < 0 ) return retval;

	return ( PAPI_OK );
}


/*
 * Native Event functions
 */
int
NWUNIT_ntv_enum_events( unsigned int *EventCode, int modifier )
{
	//printf( "NWUNIT_ntv_enum_events\n" );

	switch ( modifier ) {
	case PAPI_ENUM_FIRST:
		*EventCode = 0;

		return ( PAPI_OK );
		break;

	case PAPI_ENUM_EVENTS:
	{
		int index = ( *EventCode ) + OFFSET;

		if ( index < NWUNIT_MAX_EVENTS ) {
			*EventCode = *EventCode + 1;
			return ( PAPI_OK );
		} else
			return ( PAPI_ENOEVNT );

		break;
	}
	default:
		return ( PAPI_EINVAL );
	}
	return ( PAPI_EINVAL );
}


/*
 *
 */
int
NWUNIT_ntv_name_to_code( char *name, unsigned int *event_code )
{
#ifdef DEBUG_BGQ
	printf( "NWUNIT_ntv_name_to_code\n" );
#endif
	int ret;
	
	/* Return event id matching a given event label string */
	ret = Bgpm_GetEventIdFromLabel ( name );
	
	if ( ret <= 0 ) {
#ifdef DEBUG_BGPM
		printf ("Error: ret value is %d for BGPM API function '%s'.\n",
				ret, "Bgpm_GetEventIdFromLabel" );
#endif
		return PAPI_ENOEVNT;
	}
	else if ( ret < OFFSET || ret > NWUNIT_MAX_EVENTS ) // not a NWUnit event
		return PAPI_ENOEVNT;
	else
		*event_code = ( ret - OFFSET ) ;
	
	return PAPI_OK;
}


/*
 *
 */
int
NWUNIT_ntv_code_to_name( unsigned int EventCode, char *name, int len )
{
#ifdef DEBUG_BGQ
	//printf( "NWUNIT_ntv_code_to_name\n" );
#endif
	int index;
	
	index = ( EventCode ) + OFFSET;

	if ( index >= MAX_COUNTERS )
		return PAPI_ENOEVNT;

	strncpy( name, Bgpm_GetEventIdLabel( index ), len );
	
	if ( name == NULL ) {
#ifdef DEBUG_BGPM
		printf ("Error: ret value is NULL for BGPM API function Bgpm_GetEventIdLabel.\n" );
#endif
		return PAPI_ENOEVNT;
	}
	
	return ( PAPI_OK );
}


/*
 *
 */
int
NWUNIT_ntv_code_to_descr( unsigned int EventCode, char *name, int len )
{
#ifdef DEBUG_BGQ
	//printf( "NWUNIT_ntv_code_to_descr\n" );
#endif
	int retval, index;
	
	index = ( EventCode ) + OFFSET;
	
	retval = Bgpm_GetLongDesc( index, name, &len );
	retval = _check_BGPM_error( retval, "Bgpm_GetLongDesc" );						 
	if ( retval < 0 ) return retval;

	return ( PAPI_OK );
}


/*
 *
 */
int
NWUNIT_ntv_code_to_bits( unsigned int EventCode, hwd_register_t * bits )
{
#ifdef DEBUG_BGQ
	printf( "NWUNIT_ntv_code_to_bits\n" );
#endif
	( void ) EventCode;
	( void ) bits;
	return ( PAPI_OK );
}


/*
 *
 */
papi_vector_t _NWunit_vector = {
	.cmp_info = {
				 /* default component information (unspecified values are initialized to 0) */
				 .name = "bgpm/NWUnit",
				 .short_name = "NWUnit",
				 .description = "Blue Gene/Q NWUnit component",
				 .num_cntrs = NWUNIT_MAX_COUNTERS,
				 .num_native_events = NWUNIT_MAX_EVENTS-OFFSET+1,
				 .num_mpx_cntrs = NWUNIT_MAX_COUNTERS,
				 .default_domain = PAPI_DOM_USER,
				 .available_domains = PAPI_DOM_USER | PAPI_DOM_KERNEL,
				 .default_granularity = PAPI_GRN_THR,
				 .available_granularities = PAPI_GRN_THR,
		
				 .hardware_intr_sig = PAPI_INT_SIGNAL,
				 .hardware_intr = 1,
		
				 .kernel_multiplex = 0,

				 /* component specific cmp_info initializations */
				 .fast_real_timer = 0,
				 .fast_virtual_timer = 0,
				 .attach = 0,
				 .attach_must_ptrace = 0,
				 
				 }
	,

	/* sizes of framework-opaque component-private structures */
	.size = {
			 .context = sizeof ( NWUNIT_context_t ),
			 .control_state = sizeof ( NWUNIT_control_state_t ),
			 .reg_value = sizeof ( NWUNIT_register_t ),
			 .reg_alloc = sizeof ( NWUNIT_reg_alloc_t ),
			 }
	,
	/* function pointers in this component */
	.init_thread = NWUNIT_init_thread,
	.init_component = NWUNIT_init_component,
	.init_control_state = NWUNIT_init_control_state,
	.start = NWUNIT_start,
	.stop = NWUNIT_stop,
	.read = NWUNIT_read,
	.shutdown_thread = NWUNIT_shutdown_thread,
	.cleanup_eventset = NWUNIT_cleanup_eventset,
	.ctl = NWUNIT_ctl,

	.update_control_state = NWUNIT_update_control_state,
	.set_domain = NWUNIT_set_domain,
	.reset = NWUNIT_reset,

	.ntv_name_to_code = NWUNIT_ntv_name_to_code,
	.ntv_enum_events = NWUNIT_ntv_enum_events,
	.ntv_code_to_name = NWUNIT_ntv_code_to_name,
	.ntv_code_to_descr = NWUNIT_ntv_code_to_descr,
	.ntv_code_to_bits = NWUNIT_ntv_code_to_bits
};
