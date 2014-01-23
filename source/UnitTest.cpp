/*
	Fiedler's Cubes
	Copyright © 2008-2009 Glenn Fiedler
	http://www.gafferongames.com/fiedlers-cubes
*/

#define VALIDATE
#define SLOW_VALIDATION
#define NET_UNIT_TEST

#include <stdio.h>
#include <stdlib.h>
#include <execinfo.h>
#include <assert.h>

void print_trace()
{
	void *array[10];
	size_t size;
	char **strings;
	size_t i;

	size = backtrace( array, 10 );
	strings = backtrace_symbols( array, size );

	printf( "Obtained %zd stack frames.\n", size );

	for ( i = 0; i < size; i++ )
		printf( "%s\n", strings[i] );

	free( strings );
}

//#define assert( expression ) do { if ( !(expression) ) { print_trace(); exit(1); } } while(0)

#include "Activation.h"
#include "Engine.h"
#include "Game.h"
#include "Cubes.h"
#include "Network.h"
#include <unistd.h>

using namespace net;
using namespace std;
using namespace engine;

#include "UnitTest++/UnitTest++.h"

int main()
{
	if ( !InitializeSockets() )
		return -1;
	int result = UnitTest::RunAllTests();
	ShutdownSockets();
	return result;
}

// ----------------------------------------------------------------------------------------

SUITE( Activation )
{
	TEST( activation_system_initial_conditions )
	{
		printf( "activation system initial conditions\n" );
		const float activation_radius = 10.0f;
		const int grid_width = 20;
		const int grid_height = 20;
		const int cell_size = 1;

		activation::ActivationSystem activationSystem( 1024, activation_radius, grid_width, grid_height, cell_size, 32, 32 );
		
		CHECK( activationSystem.GetEventCount() == 0 );
		CHECK( activationSystem.GetX() == 0.0f );
		CHECK( activationSystem.GetY() == 0.0f );
		CHECK( activationSystem.GetActiveCount() == 0 );
		
		activationSystem.Validate();
		
		CHECK( activationSystem.GetWidth() == grid_width );
		CHECK( activationSystem.GetHeight() == grid_height );
		CHECK( activationSystem.GetCellSize() == cell_size );

		CHECK( activationSystem.IsEnabled() == true );
	}
		
	TEST( activation_system_activate_deactivate )
	{
		printf( "activation system activate/deactivate\n" );

		// first we setup the activation system and add some objects to it
		
		const float activation_radius = 10.0f;
		const int grid_width = 100;
		const int grid_height = 100;
		const int cell_size = 1;

		activation::ActivationSystem activationSystem( 1024, activation_radius, grid_width, grid_height, cell_size, 32, 32 );
		int id = 1;
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float(-1.0f, 0.0f ), math::random_float(-1.0f, 0.0f ) );
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float( 0.0f,+1.0f ), math::random_float(-1.0f, 0.0f ) );
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float(-1.0f, 0.0f ), math::random_float( 0.0f, 1.0f ) );
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float( 0.0f,+1.0f ), math::random_float( 0.0f, 1.0f ) );

		// verify that objects activate and we receive activation events 
		// for each object in the order we expect

		for ( int i = 0; i < 10; ++i )
			activationSystem.Update( 0.1f );
		CHECK( activationSystem.GetActiveCount() == 40 );
		CHECK( activationSystem.GetEventCount() == 40 );
		const int eventCount = activationSystem.GetEventCount();
		for ( int i = 0; i < eventCount; ++i )
		{
			const activation::Event & event = activationSystem.GetEvent(i);
			CHECK( event.type == activation::Event::Activate );
			CHECK( event.id == (activation::ObjectId) ( i + 1 ) );
		}
		activationSystem.ClearEvents();
		activationSystem.Validate();
		for ( int i = 1; i <= 40; ++i )
			CHECK( activationSystem.IsActive(i) );
	
		// now if we move the activation point really far away,
		// the objects should deactivate...
		{
			CHECK( activationSystem.GetEventCount() == 0 );
			activationSystem.MoveActivationPoint( 1000.0f, 1000.0f );
			for ( int i = 0; i < 10; ++i )
				activationSystem.Update( 0.1f );
			CHECK( activationSystem.GetActiveCount() == 0 );
			CHECK( activationSystem.GetEventCount() == 40 );
			const int eventCount = activationSystem.GetEventCount();
			for ( int i = 0; i < eventCount; ++i )
			{
				const activation::Event & event = activationSystem.GetEvent(i);
				CHECK( event.type == activation::Event::Deactivate );
				CHECK( event.id >= 1 );
				CHECK( event.id <= 40 );
			}
			activationSystem.ClearEvents();
			activationSystem.Validate();
			for ( int i = 1; i <= 40; ++i )
				CHECK( !activationSystem.IsActive(i) );
		}
		
		// when we move back to the origin we expect the objects to reactivate
		{
			activationSystem.MoveActivationPoint( 0, 0 );
			for ( int i = 0; i < 10; ++i )
				activationSystem.Update( 0.1f );
			CHECK( activationSystem.GetActiveCount() == 40 );
			CHECK( activationSystem.GetEventCount() == 40 );
			const int eventCount = activationSystem.GetEventCount();
			for ( int i = 0; i < eventCount; ++i )
			{
				const activation::Event & event = activationSystem.GetEvent(i);
				CHECK( event.type == activation::Event::Activate );
				CHECK( event.id >= 1 );
				CHECK( event.id <= 40 );
			}
			activationSystem.ClearEvents();
			activationSystem.Validate();
			for ( int i = 1; i <= 40; ++i )
				CHECK( activationSystem.IsActive(i) );
		}

		// move objects from one grid cell to another and verify they stay active
	
		CHECK( activationSystem.IsActive( 1 ) );
		activationSystem.MoveObject( 1, 0.5f, -0.5f );
		activationSystem.Validate();
		CHECK( activationSystem.IsActive( 1 ) );
		CHECK( activationSystem.GetActiveCount() == 40 );
		CHECK( activationSystem.GetEventCount() == 0 );

		CHECK( activationSystem.IsActive( 11 ) );
		activationSystem.MoveObject( 11, -0.5f, -0.5f );
		activationSystem.Validate();
		CHECK( activationSystem.IsActive( 11 ) );
		CHECK( activationSystem.GetActiveCount() == 40 );
		CHECK( activationSystem.GetEventCount() == 0 );
		
		// move an active object out of the activation circle and verify it deactivates

		CHECK( activationSystem.IsActive( 1 ) );
		activationSystem.MoveObject( 1, -15.0f, -15.0f );
		for ( int i = 0; i < 10; ++i )
		{
			activationSystem.Validate();
			activationSystem.Update( 0.1f );
		}
		CHECK( !activationSystem.IsActive( 1 ) );
		CHECK( activationSystem.GetActiveCount() == 39 );
		CHECK( activationSystem.GetEventCount() == 1 );
		if ( activationSystem.GetEventCount() == 1 )
		{
			const activation::Event & event = activationSystem.GetEvent(0);
			CHECK( event.type == activation::Event::Deactivate );
			CHECK( event.id == 1 );
		}
		activationSystem.ClearEvents();
		
		// move the deactivated object into the activation circle and verify it reactivates

		CHECK( !activationSystem.IsActive( 1 ) );
		activationSystem.MoveObject( 1, 0.0f, 0.0f );
		for ( int i = 0; i < 10; ++i )
		{
			activationSystem.Validate();
			activationSystem.Update( 0.1f );
		}
		CHECK( activationSystem.IsActive( 1 ) );
		CHECK( activationSystem.GetActiveCount() == 40 );
		CHECK( activationSystem.GetEventCount() == 1 );
		if ( activationSystem.GetEventCount() == 1 )
		{
			const activation::Event & event = activationSystem.GetEvent(0);
			CHECK( event.type == activation::Event::Activate );
			CHECK( event.id == 1 );
		}
		activationSystem.ClearEvents();
		activationSystem.Validate();
	}
	
	TEST( activation_system_enable_disable )
	{
		printf( "activation system enable disable\n" );
		
		// first we setup the activation system and add some objects to it
		
		const float activation_radius = 10.0f;
		const int grid_width = 20;
		const int grid_height = 20;
		const int cell_size = 1;

		activation::ActivationSystem activationSystem( 1024, activation_radius, grid_width, grid_height, cell_size, 32, 32 );
		int id = 1;
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float(-1.0f, 0.0f ), math::random_float(-1.0f, 0.0f ) );
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float( 0.0f,+1.0f ), math::random_float(-1.0f, 0.0f ) );
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float(-1.0f, 0.0f ), math::random_float( 0.0f, 1.0f ) );
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float( 0.0f,+1.0f ), math::random_float( 0.0f, 1.0f ) );

		// now we disable the activation system, pump updates and verify nothing activates...

		activationSystem.SetEnabled( false );
		for ( int i = 0; i < 10; ++i )
			activationSystem.Update( 0.1f );
		CHECK( activationSystem.GetActiveCount() == 0 );
		CHECK( activationSystem.GetEventCount() == 0 );
		
		// enable the activation system, pump updates, objects should now activate...

		activationSystem.SetEnabled( true );
		for ( int i = 0; i < 10; ++i )
			activationSystem.Update( 0.1f );
		CHECK( activationSystem.GetActiveCount() == 40 );
		CHECK( activationSystem.GetEventCount() == 40 );
		const int eventCount = activationSystem.GetEventCount();
		for ( int i = 0; i < eventCount; ++i )
		{
			const activation::Event & event = activationSystem.GetEvent(i);
			CHECK( event.type == activation::Event::Activate );
			CHECK( event.id == (activation::ObjectId) ( i + 1 ) );
		}
		activationSystem.ClearEvents();
		activationSystem.Validate();
		for ( int i = 1; i <= 40; ++i )
			CHECK( activationSystem.IsActive(i) );
			
		// disable the activation system, pump updates, all objects should deactivate
		{
			activationSystem.SetEnabled( false );
			for ( int i = 0; i < 10; ++i )
				activationSystem.Update( 0.1f );
			CHECK( activationSystem.GetActiveCount() == 0 );
			CHECK( activationSystem.GetEventCount() == 40 );
			const int eventCount = activationSystem.GetEventCount();
			for ( int i = 0; i < eventCount; ++i )
			{
				const activation::Event & event = activationSystem.GetEvent(i);
				CHECK( event.type == activation::Event::Deactivate );
				CHECK( event.id >= 1 );
				CHECK( event.id <= 40 );
			}
			activationSystem.ClearEvents();
			activationSystem.Validate();
			for ( int i = 1; i <= 40; ++i )
				CHECK( !activationSystem.IsActive(i) );
		}
	}

	TEST( activation_system_sweep )
	{
		printf( "activation system sweep\n" );

		// first we setup the activation system and add some objects to it
		
		const float activation_radius = 10.0f;
		const int grid_width = 50;
		const int grid_height = 50;
		const int cell_size = 1;

		activation::ActivationSystem activationSystem( 1024, activation_radius, grid_width, grid_height, cell_size, 32, 32 );
		int id = 1;
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float(-1.0f, 0.0f ), math::random_float(-1.0f, 0.0f ) );
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float( 0.0f,+1.0f ), math::random_float(-1.0f, 0.0f ) );
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float(-1.0f, 0.0f ), math::random_float( 0.0f, 1.0f ) );
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float( 0.0f,+1.0f ), math::random_float( 0.0f, 1.0f ) );

		// now we do a slow sweep from left to right and verify objects activate and deactivate *once* only
		
		bool activated[40];
		memset( activated, 0, sizeof(activated) );
		for ( float x = -100.0f; x < +100.0f; x += 0.1f )
		{
			activationSystem.MoveActivationPoint( x, 0.0f );
			activationSystem.Update( 0.1f );
			const int eventCount = activationSystem.GetEventCount();
			for ( int i = 0; i < eventCount; ++i )
			{
				const activation::Event & event = activationSystem.GetEvent(i);
				CHECK( event.id >= 1 );
				CHECK( event.id <= 40 );
				if ( event.type == activation::Event::Activate )
				{
					CHECK( !activated[event.id-1] );
					activated[event.id-1] = true;
				}
				else if ( event.type == activation::Event::Deactivate )
				{
					CHECK( activated[event.id-1] );
					activated[event.id-1] = false;
				}
			}
			activationSystem.ClearEvents();
		}
		
		// now we expect all objects to be inactive, and to have no pending events
		
		for ( int i = 0; i < 40; ++i )
			CHECK( activated[i] == false );
			
		CHECK( activationSystem.GetEventCount() == 0 );
		CHECK( activationSystem.GetActiveCount() == 0 );
	}

	TEST( activation_system_stress_test )
	{
		printf( "activation system stress test\n" );

		// first we setup the activation system and add some objects to it
		
		const float activation_radius = 10.0f;
		const int grid_width = 40;
		const int grid_height = 40;			// note: this asserts out if we reduce to 20x20 (need to switch to fixed point...)
		const int cell_size = 1;

		activation::ActivationSystem activationSystem( 1024, activation_radius, grid_width, grid_height, cell_size, 32, 32 );
		int id = 1;
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float(-1.0f, 0.0f ), math::random_float(-1.0f, 0.0f ) );
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float( 0.0f,+1.0f ), math::random_float(-1.0f, 0.0f ) );
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float(-1.0f, 0.0f ), math::random_float( 0.0f, 1.0f ) );
		for ( int i = 0; i < 10; ++i )
			activationSystem.InsertObject( id++, math::random_float( 0.0f,+1.0f ), math::random_float( 0.0f, 1.0f ) );

		// randomly move objects and the activation point in an attempt to break the system...
	
		for ( int i = 0; i < 100; ++i )
		{
			activationSystem.Update( 0.1f );

			if ( math::chance( 0.1 ) )
				activationSystem.MoveActivationPoint( math::random_float( -20.0f, +20.0f ), math::random_float( -20.0f, +20.0f ) );

			int numMoves = math::random( 20 );
			for ( int i = 0; i < numMoves; ++i )
			{
				int id = 1 + math::random( 40 );
				assert( id >=  1 );
				assert( id <= 40 );
				activationSystem.MoveObject( id, math::random_float( -20.0f, +20.0f ), math::random_float( -20.0f, +20.0f ) );
			}

			activationSystem.Validate();
			
			activationSystem.ClearEvents();
		}

		// disable the activation system, pump updates, all objects should deactivate

		activationSystem.SetEnabled( false );
		for ( int i = 0; i < 10; ++i )
			activationSystem.Update( 0.1f );
		CHECK( activationSystem.GetActiveCount() == 0 );
		const int eventCount = activationSystem.GetEventCount();
		for ( int i = 0; i < eventCount; ++i )
		{
			const activation::Event & event = activationSystem.GetEvent(i);
			CHECK( event.type == activation::Event::Deactivate );
			CHECK( event.id >= 1 );
			CHECK( event.id <= 40 );
		}
		activationSystem.ClearEvents();
		activationSystem.Validate();
		for ( int i = 1; i <= 40; ++i )
			CHECK( !activationSystem.IsActive(i) );
	}
}

// ------------------------------------------------------------------------------------------------------

SUITE( Authority )
{
	TEST( authority_manager_initial_conditions )
	{
		printf( "authority manager initial conditions\n" );

		AuthorityManager authorityManager;
		CHECK( authorityManager.GetEntryCount() == 0 );
		for ( int i = 1; i <= 40; ++i )
			CHECK( authorityManager.GetAuthority(i) == MaxPlayers );
	}
	
	TEST( authority_manager_set_authority )
	{
		printf( "authority manager set authority\n" );

		AuthorityManager authorityManager;
		for ( int i = 1; i <= 40; ++i )
			CHECK( authorityManager.SetAuthority( i, 0 ) );
		for ( int i = 1; i <= 40; ++i )
			CHECK( authorityManager.GetAuthority( i ) == 0 );
	}

	TEST( authority_manager_clear )
	{
		printf( "authority manager clear\n" );

		AuthorityManager authorityManager;
		for ( int i = 1; i <= 40; ++i )
			CHECK( authorityManager.SetAuthority( i, 0 ) );
		for ( int i = 1; i <= 40; ++i )
			CHECK( authorityManager.GetAuthority( i ) == 0 );

		authorityManager.Clear();

		CHECK( authorityManager.GetEntryCount() == 0 );
		for ( int i = 1; i <= 40; ++i )
			CHECK( authorityManager.GetAuthority(i) == MaxPlayers );
	}

	TEST( authority_manager_win_tie_break )
	{
		printf( "authority manager win tie break\n" );

		AuthorityManager authorityManager;
		for ( int i = 1; i <= 40; ++i )
			CHECK( authorityManager.SetAuthority( i, 1 ) );
		for ( int i = 1; i <= 40; ++i )
			CHECK( authorityManager.SetAuthority( i, 0 ) );
		for ( int i = 1; i <= 40; ++i )
			CHECK( authorityManager.GetAuthority( i ) == 0 );
	}

	TEST( authority_manager_lose_tie_break )
	{
		printf( "authority manager lose tie break\n" );

		AuthorityManager authorityManager;
		for ( int i = 1; i <= 40; ++i )
			CHECK( authorityManager.SetAuthority( i, 0 ) );
		for ( int i = 1; i <= 40; ++i )
			CHECK( authorityManager.SetAuthority( i, 1 ) == false );
		for ( int i = 1; i <= 40; ++i )
			CHECK( authorityManager.GetAuthority( i ) == 0 );
	}

	TEST( authority_manager_force_authority )
	{
		printf( "authority manager force authority\n" );

		AuthorityManager authorityManager;
		for ( int i = 1; i <= 40; ++i )
			CHECK( authorityManager.SetAuthority( i, 0 ) );
		for ( int i = 1; i <= 40; ++i )
			CHECK( authorityManager.SetAuthority( i, 1, true ) );
		for ( int i = 1; i <= 40; ++i )
			CHECK( authorityManager.GetAuthority( i ) == 1 );
	}

	TEST( authority_manager_default_authority )
	{
		printf( "authority manager default authority\n" );

		AuthorityManager authorityManager;
		for ( int i = 1; i <= 40; ++i )
		{
			CHECK( authorityManager.SetAuthority( i, 1 ) );
		}
		for ( int i = 0; i < 100; ++i )
		{
			authorityManager.Update( 1.0f, 2.0f );
		}
		for ( int i = 1; i <= 40; ++i )
			CHECK( authorityManager.GetAuthority( i ) == MaxPlayers );
	}
}

SUITE( InteractionManager )
{
	TEST( interaction_manager_initial_conditions )
	{
		printf( "interaction manager initial conditions\n" );

		InteractionManager interactionManager;
		int numObjects = 100;
		interactionManager.PrepInteractions( numObjects );
		for ( int i = 0; i < numObjects; ++i )
			CHECK( !interactionManager.IsInteracting( i ) );
	}

	TEST( interaction_manager_walk_interactions )
	{
		printf( "interaction manager walk interactions\n" );

		InteractionManager interactionManager;
		
		int numObjects = 200;
		interactionManager.PrepInteractions( numObjects );
		
		// here we assume that a,b,c,d are touching
		// the active ids of abcd are as follows...
		const int a = 10;
		const int b = 17;
		const int c = 100;
		const int d = 23;

		const int numInteractionPairs = 4;
		InteractionPair interactionPairs[numInteractionPairs];

		interactionPairs[0].a = a;
		interactionPairs[0].b = b;
		
		interactionPairs[1].a = b;
		interactionPairs[1].b = c;

		interactionPairs[2].a = c;
		interactionPairs[2].b = d;

		interactionPairs[3].a = d;
		interactionPairs[3].b = a;

		// walk interactions starting with a
		std::vector<bool> ignores( numObjects, false );
		interactionManager.WalkInteractions( a, interactionPairs, numInteractionPairs, ignores );
		
		// verify abcd are interacting
		CHECK( interactionManager.IsInteracting( a ) );
		CHECK( interactionManager.IsInteracting( b ) );
		CHECK( interactionManager.IsInteracting( c ) );
		CHECK( interactionManager.IsInteracting( d ) );

		// verify rest of objects are not interacting
		for ( int i = 0; i < numObjects; ++i )
		{
			if ( i == a || i == b || i == c || i == d )
				continue;
			CHECK( !interactionManager.IsInteracting( i ) );
		}
	}

	TEST( interaction_manager_ignore )
	{
		printf( "interaction manager ignore\n" );

		InteractionManager interactionManager;
		
		int numObjects = 200;
		interactionManager.PrepInteractions( numObjects );

		// here we assume that a,b,c,d are touching *and* we setup c as an ignore
		// the goal is for only ab to be interacting and no others
		const int a = 10;
		const int b = 17;
		const int c = 100;
		const int d = 23;

		const int numInteractionPairs = 3;
		InteractionPair interactionPairs[numInteractionPairs];

		interactionPairs[0].a = a;
		interactionPairs[0].b = b;
		
		interactionPairs[1].a = b;
		interactionPairs[1].b = c;

		interactionPairs[2].a = c;
		interactionPairs[2].b = d;

		// set c to ignore
		std::vector<bool> ignores( numObjects, false );
		ignores[c] = true;

		// walk interactions starting with a, we expect c to break the chain of interactions
		interactionManager.WalkInteractions( a, interactionPairs, numInteractionPairs, ignores );
		
		// verify only ab are interacting
		CHECK( interactionManager.IsInteracting( a ) );
		CHECK( interactionManager.IsInteracting( b ) );
		CHECK( !interactionManager.IsInteracting( c ) );
		CHECK( !interactionManager.IsInteracting( d ) );
	}
}

SUITE( ResponseQueue )
{
	struct Response
	{
		ObjectId id;
		Response() { id = 0; }
		Response( ObjectId id ) { this->id = id; }
	};
	
	TEST( response_queue_initial_conditions )
	{
		printf( "response queue initial conditions\n" );

		ResponseQueue<Response> responseQueue;

		Response response;
		CHECK( !responseQueue.PopResponse( response ) );

		for ( int i = 0; i < 100; ++i )
			CHECK( !responseQueue.AlreadyQueued( i ) );
	}
	
	TEST( response_queue_pop )
	{
		printf( "response queue pop\n" );

		ResponseQueue<Response> responseQueue;

 		Response a = 10;
 		Response b = 15;
 		Response c = 6;

		responseQueue.QueueResponse( a );
		responseQueue.QueueResponse( b );
		responseQueue.QueueResponse( c );

		Response response;
		CHECK( responseQueue.PopResponse( response ) );
		CHECK( response.id == a.id );

		CHECK( responseQueue.PopResponse( response ) );
		CHECK( response.id == b.id );

		CHECK( responseQueue.PopResponse( response ) );
		CHECK( response.id == c.id );
	}

	TEST( response_queue_clear )
	{
		printf( "response queue clear\n" );

		ResponseQueue<Response> responseQueue;

 		Response a = 10;
 		Response b = 15;
 		Response c = 6;

		responseQueue.QueueResponse( a );
		responseQueue.QueueResponse( b );
		responseQueue.QueueResponse( c );

		responseQueue.Clear();

		Response response;
		CHECK( !responseQueue.PopResponse( response ) );

		for ( int i = 0; i < 100; ++i )
			CHECK( !responseQueue.AlreadyQueued( i ) );
	}
}

SUITE( PrioritySet )
{
	TEST( priority_set_initial_conditions )
	{
		printf( "priority set initial conditions\n" );

		engine::PrioritySet prioritySet;
		CHECK( prioritySet.GetObjectCount() == 0 );
	}

	TEST( priority_add_remove_clear )
	{
		printf( "priority set add/remove/clear\n" );

		engine::PrioritySet prioritySet;
		prioritySet.AddObject( 1 );
		prioritySet.AddObject( 2 );
		prioritySet.AddObject( 3 );
		prioritySet.AddObject( 4 );
		prioritySet.AddObject( 5 );
		prioritySet.RemoveObject( 3 );
		prioritySet.AddObject( 6 );
		CHECK( prioritySet.GetObjectCount() == 5 );
		CHECK( prioritySet.GetPriorityObject( 0 ) == 1 );
		CHECK( prioritySet.GetPriorityObject( 1 ) == 2 );
		CHECK( prioritySet.GetPriorityObject( 2 ) == 5 );
		CHECK( prioritySet.GetPriorityObject( 3 ) == 4 );
		CHECK( prioritySet.GetPriorityObject( 4 ) == 6 );
		prioritySet.Clear();
		CHECK( prioritySet.GetObjectCount() == 0 );
	}

	TEST( priority_sort_objects )
	{
		printf( "priority set sort objects\n" );

		engine::PrioritySet prioritySet;
		
		for ( int i = 0; i < 6; ++i )
			prioritySet.AddObject( i + 1 );

		prioritySet.SetPriorityAtIndex( 0, 0.5f );
		prioritySet.SetPriorityAtIndex( 1, 0.1f );
		prioritySet.SetPriorityAtIndex( 2, 1.0f );
		prioritySet.SetPriorityAtIndex( 3, 0.7f );
		prioritySet.SetPriorityAtIndex( 4, 1000.0f );
		prioritySet.SetPriorityAtIndex( 5, 100.0f );
		
		prioritySet.SortObjects();
		
		CHECK( prioritySet.GetPriorityObject(0) == 5 );
		CHECK( prioritySet.GetPriorityObject(1) == 6 );
		CHECK( prioritySet.GetPriorityObject(2) == 3 );
		CHECK( prioritySet.GetPriorityObject(3) == 4 );
		CHECK( prioritySet.GetPriorityObject(4) == 1 );
		CHECK( prioritySet.GetPriorityObject(5) == 2 );
		
		CHECK_EQUAL( prioritySet.GetPriorityAtIndex(0), 1000.0f );
		CHECK_EQUAL( prioritySet.GetPriorityAtIndex(1), 100.0f );
		CHECK_EQUAL( prioritySet.GetPriorityAtIndex(2), 1.0f );
		CHECK_EQUAL( prioritySet.GetPriorityAtIndex(3), 0.7f );
		CHECK_EQUAL( prioritySet.GetPriorityAtIndex(4), 0.5f );
		CHECK_EQUAL( prioritySet.GetPriorityAtIndex(5), 0.1f );
		
		prioritySet.SetPriorityAtIndex( 0, 0.0f );
		prioritySet.SortObjects();

		CHECK( prioritySet.GetPriorityObject(0) == 6 );
		CHECK( prioritySet.GetPriorityObject(1) == 3 );
		CHECK( prioritySet.GetPriorityObject(2) == 4 );
		CHECK( prioritySet.GetPriorityObject(3) == 1 );
		CHECK( prioritySet.GetPriorityObject(4) == 2 );
		CHECK( prioritySet.GetPriorityObject(5) == 5 );
		
		CHECK_EQUAL( prioritySet.GetPriorityAtIndex(0), 100.0f );
		CHECK_EQUAL( prioritySet.GetPriorityAtIndex(1), 1.0f );
		CHECK_EQUAL( prioritySet.GetPriorityAtIndex(2), 0.7f );
		CHECK_EQUAL( prioritySet.GetPriorityAtIndex(3), 0.5f );
		CHECK_EQUAL( prioritySet.GetPriorityAtIndex(4), 0.1f );
		CHECK_EQUAL( prioritySet.GetPriorityAtIndex(5), 0.0f );
	}
}

SUITE( Compression )
{
	TEST( compress_position )
	{
		printf( "compress position\n" );

		math::Vector input(10,100,200.5f);
		math::Vector output(0,0,0);
		uint64_t compressed;
		game::CompressPosition( input, compressed );
		game::DecompressPosition( compressed, output );
		CHECK_CLOSE( input.x, output.x, 0.001f );
		CHECK_CLOSE( input.y, output.y, 0.001f );
		CHECK_CLOSE( input.z, output.z, 0.001f );
	}

	TEST( compress_orientation )
	{
		printf( "compress orientation\n" );

		math::Quaternion input(1,0,0,0);
		math::Quaternion output(0,0,0,0);
		uint32_t compressed;
		game::CompressOrientation( input, compressed );
		game::DecompressOrientation( compressed, output );
		CHECK_CLOSE( input.w, output.w, 0.001f );
		CHECK_CLOSE( input.x, output.x, 0.001f );
		CHECK_CLOSE( input.y, output.y, 0.001f );
		CHECK_CLOSE( input.z, output.z, 0.001f );
	}
}

SUITE( Game )
{
	TEST( game_initial_conditions )
	{
		printf( "game initial conditions\n" );

		game::Instance<cubes::DatabaseObject, cubes::ActiveObject> instance;
		CHECK( instance.GetLocalPlayer() == -1 );
		for ( int i = 0; i < MaxPlayers; ++i )
		{
			CHECK( !instance.IsPlayerJoined(i) );
			game::Input input;
			instance.GetPlayerInput( i, input );
			CHECK( input == game::Input() );
			CHECK( instance.GetPlayerFocus( i ) == 0 );
		}
		CHECK( instance.GetLocalPlayer() == -1 );
		CHECK( !instance.InGame() );
	}
	
	TEST( game_initialize_shutdown )
	{
		printf( "game initialize/shutdown\n" );

		game::Config config;
		config.cellSize = 4.0f;
		config.cellWidth = 16;
		config.cellHeight = 16;

		game::Instance<cubes::DatabaseObject, cubes::ActiveObject> instance( config );

		for ( int i = 0; i < 2; ++i )
		{
			instance.InitializeBegin();
			instance.InitializeEnd();
			instance.Shutdown();
		}
	}
	
	void AddCube( game::Instance<cubes::DatabaseObject, cubes::ActiveObject> * gameInstance, float scale, const math::Vector & position, const math::Vector & linearVelocity = math::Vector(0,0,0), const math::Vector & angularVelocity = math::Vector(0,0,0) )
	{
		cubes::DatabaseObject object;
		object.position = position;
		object.orientation = math::Quaternion(1,0,0,0);
		object.scale = scale;
		object.linearVelocity = linearVelocity;
		object.angularVelocity = angularVelocity;
		object.enabled = 1;
		object.activated = 0;
		gameInstance->AddObject( object, position.x, position.y );
	}

	TEST( game_player_join_and_leave )
	{
		printf( "game join and leave\n" );

		game::Config config;
		config.cellSize = 4.0f;
		config.cellWidth = 16;
		config.cellHeight = 16;

		game::Instance<cubes::DatabaseObject, cubes::ActiveObject> instance( config );
		
		instance.InitializeBegin();
		AddCube( &instance, 1.0f, math::Vector(0,0,0) );
		AddCube( &instance, 1.0f, math::Vector(0,0,0) );
		AddCube( &instance, 1.0f, math::Vector(0,0,0) );
		AddCube( &instance, 1.0f, math::Vector(0,0,0) );
		instance.InitializeEnd();
		
		for ( int i = 0; i < MaxPlayers; ++i )
		{
			CHECK( !instance.IsPlayerJoined( i ) );
			instance.OnPlayerJoined( i );
			instance.SetPlayerFocus( i, i + 1 );
			CHECK( instance.IsPlayerJoined( i ) );
			CHECK( instance.GetPlayerFocus( i ) == ObjectId( i + 1 ) );
		}

		CHECK( !instance.InGame() );
		CHECK( instance.GetLocalPlayer() == -1 );
		instance.SetLocalPlayer( 1 );
		CHECK( instance.GetLocalPlayer() == 1 );
		CHECK( instance.InGame() );		
		
		for ( int i = 0; i < MaxPlayers; ++i )
		{
			CHECK( instance.IsPlayerJoined( i ) );
			instance.OnPlayerLeft( i );
			CHECK( !instance.IsPlayerJoined( i ) );
		}
		
		instance.Shutdown();

		CHECK( instance.GetLocalPlayer() == -1 );
		CHECK( !instance.InGame() );

		for ( int i = 0; i < MaxPlayers; ++i )
		{
			CHECK( !instance.IsPlayerJoined( i ) );
			CHECK( instance.GetPlayerFocus( i ) == 0 );
		}
	}
	
	TEST( game_object_activation )
	{
		printf( "game object activation\n" );

		game::Config config;
		config.cellSize = 4.0f;
		config.cellWidth = 16;
		config.cellHeight = 16;

		game::Instance<cubes::DatabaseObject, cubes::ActiveObject> instance( config );
		
		instance.InitializeBegin();
		AddCube( &instance, 1.0f, math::Vector(0,0,0) );
		instance.InitializeEnd();

		instance.SetFlag( game::FLAG_Pause );

		instance.OnPlayerJoined( 0 );
		instance.SetPlayerFocus( 0, 1 );
		instance.SetLocalPlayer( 0 );

		instance.Update();
		
		int numActiveObjects = 0;
		cubes::ActiveObject activeObjects[1];
		instance.GetActiveObjects( activeObjects, numActiveObjects );
		CHECK( numActiveObjects == 1 );
		CHECK( instance.IsObjectActive( 1 ) );

		instance.OnPlayerLeft( 0 );

		instance.Update();
		
		instance.GetActiveObjects( activeObjects, numActiveObjects );
		CHECK( numActiveObjects == 0 );
		CHECK( !instance.IsObjectActive( 1 ) );
	}

	TEST( game_object_get_set_state )
	{
		printf( "game object get/set state\n" );

		game::Config config;
		config.cellSize = 4.0f;
		config.cellWidth = 16;
		config.cellHeight = 16;

		game::Instance<cubes::DatabaseObject, cubes::ActiveObject> instance( config );
		
		instance.InitializeBegin();
		for ( int i = 0; i < 20; ++i )
		{
			cubes::DatabaseObject object;
			object.position = math::Vector(0,0,0);
			object.orientation = math::Quaternion(1,0,0,0);
			object.scale = ( i == 0 ) ? 1.4f : 0.4f;
			object.linearVelocity = math::Vector(0,0,0);
			object.angularVelocity = math::Vector(0,0,0);
			object.enabled = 1;
			object.activated = 0;
			instance.AddObject( object, 0, 0 );
		}
		
		instance.InitializeEnd();

		instance.SetFlag( game::FLAG_Pause );

		// join and activate some objects

		instance.OnPlayerJoined( 0 );
		instance.SetLocalPlayer( 0 );
		instance.SetPlayerFocus( 0, 1 );

		instance.Update();

		int numActiveObjects = 0;
		cubes::ActiveObject activeObjects[32];
		instance.GetActiveObjects( activeObjects, numActiveObjects );
		CHECK( numActiveObjects > 0 );
		
		// move active objects outside activation distance (except player objects...)
		
		for ( int i = 0; i < numActiveObjects; ++i )
		{
			if ( activeObjects[i].scale < 1.0f )
			{
				activeObjects[i].position.x = 20.0;
				activeObjects[i].position.y = 20.0;
				instance.SetObjectState( activeObjects[i].id, activeObjects[i] );
			}
		}
		
		// verify all non-player objects have deactivated
		
		instance.Update();

		int after_numActiveObjects = 0;
		cubes::ActiveObject after_activeObjects[32];
		instance.GetActiveObjects( after_activeObjects, after_numActiveObjects );
		CHECK( after_numActiveObjects == 1 );
		CHECK( after_activeObjects[0].scale > 1.0f );
			
		// move the objects back in to the origin point

		const math::Vector origin = instance.GetOrigin();
		for ( int i = 0; i < numActiveObjects; ++i )
		{
			if ( activeObjects[i].scale < 1.0f )
			{
				activeObjects[i].position.x = origin.x;
				activeObjects[i].position.y = origin.y;
				instance.SetObjectState( activeObjects[i].id, activeObjects[i] );
			}
		}
		
		// verify they activate again

		instance.Update();
		
		int newActiveCount = instance.GetActiveObjectCount();
		CHECK( newActiveCount == numActiveObjects );
		for ( int i = 0; i < numActiveObjects; ++i )
		{
			CHECK( instance.IsObjectActive( activeObjects[i].id ) );
		}		
	}

	TEST( game_object_persistence )
	{
		printf( "game object persistence\n" );

		// make sure objects remember their state when they re-activate
		
		game::Config config;
		config.cellSize = 4.0f;
		config.cellWidth = 16;
		config.cellHeight = 16;

		game::Instance<cubes::DatabaseObject, cubes::ActiveObject> instance( config );
		
		instance.InitializeBegin();
		for ( int i = 0; i < 20; ++i )
		{
			cubes::DatabaseObject object;
			object.position = math::Vector( math::random_float( -10.0f, +10.0f ), math::random_float( -10.0f, +10.0f ), 5 );
			object.orientation = math::Quaternion(1,0,0,0);
			object.scale = ( i == 0 ) ? 1.4f : 0.4f;
			object.linearVelocity = math::Vector(0,0,0);
			object.angularVelocity = math::Vector(0,0,0);
			object.enabled = 1;
			object.activated = 0;
			instance.AddObject( object, object.position.x, object.position.y );
		}
		instance.InitializeEnd();

		instance.SetFlag( game::FLAG_Pause );

		// join and activate some objects

		instance.OnPlayerJoined( 0 );
		instance.SetLocalPlayer( 0 );
		instance.SetPlayerFocus( 0, 1 );

		for ( int i = 0; i < 10; ++i )
			instance.Update();

		int numActiveObjects = 0;
		cubes::ActiveObject activeObjects[32];
		instance.GetActiveObjects( activeObjects, numActiveObjects );
		CHECK( numActiveObjects > 0 );

		// verify player cube is active

		int focusObjectId = instance.GetPlayerFocus( 0 );
		CHECK( focusObjectId >= 0 );
		CHECK( instance.IsObjectActive( focusObjectId ) );
		
		// change active object positions and orientations
		
		math::Vector origin = instance.GetOrigin();

		for ( int i = 0; i < numActiveObjects; ++i )
		{
			CHECK( activeObjects[i].id >= 0 );
			if ( activeObjects[i].scale > 1.0f )
			{
				activeObjects[i].position.x = origin.x + ( activeObjects[i].position.x - origin.x ) * 0.5f;
				activeObjects[i].position.y = origin.y + ( activeObjects[i].position.y - origin.y ) * 0.5f;
				activeObjects[i].position.z = 0.0f;
				activeObjects[i].orientation = math::Quaternion( 0.5f, activeObjects[i].position.x, activeObjects[i].position.y, activeObjects[i].position.x + activeObjects[i].position.y );
				activeObjects[i].orientation.normalize();
				instance.SetObjectState( activeObjects[i].id, activeObjects[i] );
			}
		}

		// verify all objects are still active
		
		for ( int i = 0; i < 5; ++i )
			instance.Update();		
		
		CHECK( instance.GetActiveObjectCount() == numActiveObjects );

		// move player cube away, deactivate objects

		instance.OnPlayerLeft( 0 );

		for ( int i = 0; i < 5; ++i )
			instance.Update();

		CHECK( instance.GetActiveObjectCount() == 0 );

		// rejoin player

		instance.OnPlayerJoined( 0 );
		instance.SetLocalPlayer( 0 );

		for ( int i = 0; i < 5; ++i )
			instance.Update();

		// verify original objects are active and remember their positions and orientations

		int after_numActiveObjects = 0;
		cubes::ActiveObject after_activeObjects[32];
		instance.GetActiveObjects( after_activeObjects, after_numActiveObjects );

		CHECK( after_numActiveObjects == numActiveObjects );

		for ( int i = 0; i < after_numActiveObjects; ++i )
		{
			bool found = false;
			for ( int j = 0; j < numActiveObjects; ++j )
			{
				if ( after_activeObjects[i].id == activeObjects[j].id )
				{
					found = true;
					
					CHECK_CLOSE( after_activeObjects[i].position.x, activeObjects[j].position.x, 0.001f );
					CHECK_CLOSE( after_activeObjects[i].position.y, activeObjects[j].position.y, 0.001f );
					CHECK_CLOSE( after_activeObjects[i].position.z, activeObjects[j].position.z, 0.001f );
					
					const float cosine = after_activeObjects[i].orientation.w * activeObjects[j].orientation.w +
										 after_activeObjects[i].orientation.x * activeObjects[j].orientation.x +
										 after_activeObjects[i].orientation.y * activeObjects[j].orientation.y +
										 after_activeObjects[i].orientation.z * activeObjects[j].orientation.z;

					if ( cosine < 0 )
						after_activeObjects[i].orientation *= -1;
					
					CHECK_CLOSE( after_activeObjects[i].orientation.w, activeObjects[j].orientation.w, 0.03f );
					CHECK_CLOSE( after_activeObjects[i].orientation.x, activeObjects[j].orientation.x, 0.03f );
					CHECK_CLOSE( after_activeObjects[i].orientation.y, activeObjects[j].orientation.y, 0.03f );
					CHECK_CLOSE( after_activeObjects[i].orientation.z, activeObjects[j].orientation.z, 0.03f );
				}
			}
			CHECK( found );
		}
	}
	
	TEST( game_object_authority )
	{
		printf( "game object authority\n" );

		game::Config config;
		config.cellSize = 4.0f;
		config.cellWidth = 16;
		config.cellHeight = 16;

		game::Instance<cubes::DatabaseObject, cubes::ActiveObject> instance( config );
		
		instance.InitializeBegin();
		instance.AddPlane( math::Vector(0,0,1), 0 );
		for ( int i = 0; i < 20; ++i )
		{
			cubes::DatabaseObject object;
			object.position = math::Vector( 0, 0, i + 1.0f );
			object.orientation = math::Quaternion(1,0,0,0);
			object.scale = ( i == 0 ) ? 1.4f : 0.4f;
			object.linearVelocity = math::Vector(0,0,0);
			object.angularVelocity = math::Vector(0,0,0);
			object.enabled = 1;
			object.activated = 0;
			instance.AddObject( object, object.position.x, object.position.y );
		}
		instance.InitializeEnd();

		// join players

		instance.OnPlayerJoined( 0 );		
		instance.SetLocalPlayer( 0 );
		instance.SetPlayerFocus( 0, 1 );

		// verify initial authority
		{
			instance.Update( 0.1f );
		
			int numActiveObjects = 0;
			cubes::ActiveObject activeObjects[32];
			instance.GetActiveObjects( activeObjects, numActiveObjects );
			
			for ( int i = 0; i < numActiveObjects; ++i )
			{
				int authority = instance.GetObjectAuthority( activeObjects[i].id );
				if ( activeObjects[i].scale > 1.0f )
				{
					CHECK( authority == (int) ( activeObjects[i].id - 1 ) );
				}
				else
				{
					CHECK( authority == MaxPlayers );
				}
			}
		}
		
		// let the game simulate for one second
		// some cubes should have dropped onto the player cube and turned
		// to player 0 authority...
		{
			for ( int i = 0; i < 10; ++i )
				instance.Update( 0.1f );
			
			int interactionAuthorityCount = 0;

			int numActiveObjects = 0;
			cubes::ActiveObject activeObjects[32];
			instance.GetActiveObjects( activeObjects, numActiveObjects );

			for ( int i = 0; i < numActiveObjects; ++i )
			{
				int authority = instance.GetObjectAuthority( activeObjects[i].id );
				if ( activeObjects[i].scale > 1.0f )
				{
					CHECK( authority == (int) ( activeObjects[i].id - 1 ) );
				}
				else
				{
					CHECK( authority == MaxPlayers || authority == 0 );
					if ( authority == 0 )
						interactionAuthorityCount++;
				}
			}
			
			CHECK( interactionAuthorityCount >= 1 );
		}
	}
}
	
// ------------------------------------------------------------------------------------------------------

SUITE( BitPacker )
{
	TEST( write_bits )
	{
		unsigned char buffer[256];
		memset( buffer, 0, sizeof( buffer ) );
		BitPacker bitpacker( BitPacker::Write, buffer, sizeof(buffer) );

		bitpacker.WriteBits( 0xFFFFFFFF, 32 );
		CHECK( bitpacker.GetBits() == 32 );
		CHECK( bitpacker.GetBytes() == 4 );
		CHECK( buffer[0] == 0xFF );
		CHECK( buffer[1] == 0xFF );
		CHECK( buffer[2] == 0xFF );
		CHECK( buffer[3] == 0xFF );
		CHECK( buffer[4] == 0x00 );

		bitpacker.WriteBits( 0x1111FFFF, 16 );
		CHECK( bitpacker.GetBits() == 32 + 16 );
		CHECK( bitpacker.GetBytes() == 6 );
		CHECK( buffer[0] == 0xFF );
		CHECK( buffer[1] == 0xFF );
		CHECK( buffer[2] == 0xFF );
		CHECK( buffer[3] == 0xFF );
		CHECK( buffer[4] == 0xFF );
		CHECK( buffer[5] == 0xFF );
		CHECK( buffer[6] == 0x00 );

		bitpacker.WriteBits( 0x111111FF, 8 );
		CHECK( bitpacker.GetBits() == 32 + 16 + 8 );
		CHECK( bitpacker.GetBytes() == 7 );
		CHECK( buffer[0] == 0xFF );
		CHECK( buffer[1] == 0xFF );
		CHECK( buffer[2] == 0xFF );
		CHECK( buffer[3] == 0xFF );
		CHECK( buffer[4] == 0xFF );
		CHECK( buffer[5] == 0xFF );
		CHECK( buffer[6] == 0xFF );
		CHECK( buffer[7] == 0x00 );
	}

	TEST( write_bits_odd )
	{
		unsigned char buffer[256];
		memset( buffer, 0, sizeof( buffer ) );
		BitPacker bitpacker( BitPacker::Write, buffer, sizeof(buffer) );

		bitpacker.WriteBits( 0xFFFFFFFF, 9 );
		CHECK( bitpacker.GetBytes() == 2 );
		CHECK( bitpacker.GetBits() == 9 );

		bitpacker.WriteBits( 0xFFFFFFFF, 1 );
		CHECK( bitpacker.GetBytes() == 2 );
		CHECK( bitpacker.GetBits() == 9 + 1 );

		bitpacker.WriteBits( 0xFFFFFFFF, 11 );
		CHECK( bitpacker.GetBytes() == 3 );		
		CHECK( bitpacker.GetBits() == 9 + 1 + 11 );

		bitpacker.WriteBits( 0xFFFFFFFF, 6 );
		CHECK( bitpacker.GetBytes() == 4 );
		CHECK( bitpacker.GetBits() == 9 + 1 + 11 + 6 );

		bitpacker.WriteBits( 0xFFFFFFFF, 5 );
		CHECK( bitpacker.GetBytes() == 4 );
		CHECK( bitpacker.GetBits() == 32 );

		CHECK( buffer[0] == 0xFF );
		CHECK( buffer[1] == 0xFF );
		CHECK( buffer[2] == 0xFF );
		CHECK( buffer[3] == 0xFF );
		CHECK( buffer[4] == 0x00 );		
	}

	TEST( read_bits )
	{
		unsigned char buffer[256];
		memset( buffer, 0, sizeof( buffer ) );
		buffer[0] = 0xFF;
		buffer[1] = 0xFF;
		buffer[2] = 0xFF;
		buffer[3] = 0xFF;
		buffer[4] = 0xFF;
		buffer[5] = 0xFF;
		buffer[6] = 0xFF;

		BitPacker bitpacker( BitPacker::Read, buffer, sizeof(buffer) );
		unsigned int value;
		bitpacker.ReadBits( value, 32 );
		CHECK( value == 0xFFFFFFFF );
		CHECK( bitpacker.GetBytes() == 4 );
		CHECK( bitpacker.GetBits() == 32 );

		bitpacker.ReadBits( value, 16 );
		CHECK( value == 0x0000FFFF );
		CHECK( bitpacker.GetBytes() == 6 );
		CHECK( bitpacker.GetBits() == 32 + 16 );

		bitpacker.ReadBits( value, 8 );
		CHECK( value == 0x000000FF );
		CHECK( bitpacker.GetBytes() == 7 );
		CHECK( bitpacker.GetBits() == 32 + 16 + 8 );
	}

	TEST( read_bits_odd )
	{
		unsigned char buffer[256];
		memset( buffer, 0, sizeof( buffer ) );
		buffer[0] = 0xFF;
		buffer[1] = 0xFF;
		buffer[2] = 0xFF;
		buffer[3] = 0xFF;
		buffer[4] = 0x00;

		BitPacker bitpacker( BitPacker::Read, buffer, sizeof(buffer) );

		unsigned int value;
		bitpacker.ReadBits( value, 9 );
		CHECK( bitpacker.GetBytes() == 2 );
		CHECK( bitpacker.GetBits() == 9 );
		CHECK( value == ( 1 << 9 ) - 1 );

		bitpacker.ReadBits( value, 1 );
		CHECK( bitpacker.GetBytes() == 2 );
		CHECK( bitpacker.GetBits() == 9 + 1 );
		CHECK( value == 1 );

		bitpacker.ReadBits( value, 11 );
		CHECK( bitpacker.GetBytes() == 3 );
		CHECK( bitpacker.GetBits() == 9 + 1 + 11 );
		CHECK( value == ( 1 << 11 ) - 1 );

		bitpacker.ReadBits( value, 6 );
		CHECK( bitpacker.GetBytes() == 4 );
		CHECK( bitpacker.GetBits() == 9 + 1 + 11 + 6 );
		CHECK( value == ( 1 << 6 ) - 1 );

		bitpacker.ReadBits( value, 5 );
		CHECK( bitpacker.GetBytes() == 4 );
		CHECK( bitpacker.GetBits() == 32 );
		CHECK( value == ( 1 << 5 ) - 1 );
	}

	TEST( read_write_bits )
	{
		unsigned char buffer[256];
		memset( buffer, 0, sizeof( buffer ) );

		unsigned int a = 123;
		unsigned int b = 1;
		unsigned int c = 10004;
		unsigned int d = 50234;
		unsigned int e = 1020491;
		unsigned int f = 55;
		unsigned int g = 40;
		unsigned int h = 100;

		const int total_bits = 256 * 8;
		const int used_bits = 7 + 1 + 14 + 16 + 20 + 6 + 6 + 7;
		const int used_bytes = 10;

		BitPacker bitpacker( BitPacker::Write, buffer, sizeof(buffer) );
		bitpacker.WriteBits( a, 7 );
		bitpacker.WriteBits( b, 1 );
		bitpacker.WriteBits( c, 14 );
		bitpacker.WriteBits( d, 16 );
		bitpacker.WriteBits( e, 20 );
		bitpacker.WriteBits( f, 6 );
		bitpacker.WriteBits( g, 6 );
		bitpacker.WriteBits( h, 7 );
		CHECK( bitpacker.GetBits() == used_bits );
		CHECK( bitpacker.GetBytes() == used_bytes );
		CHECK( bitpacker.BitsRemaining() == total_bits - used_bits );

		unsigned int a_out = 0xFFFFFFFF;
		unsigned int b_out = 0xFFFFFFFF;
		unsigned int c_out = 0xFFFFFFFF;
		unsigned int d_out = 0xFFFFFFFF;
		unsigned int e_out = 0xFFFFFFFF;
		unsigned int f_out = 0xFFFFFFFF;
		unsigned int g_out = 0xFFFFFFFF;
		unsigned int h_out = 0xFFFFFFFF;

		bitpacker = BitPacker( BitPacker::Read, buffer, sizeof(buffer ) );
		bitpacker.ReadBits( a_out, 7 );
		bitpacker.ReadBits( b_out, 1 );
		bitpacker.ReadBits( c_out, 14 );
		bitpacker.ReadBits( d_out, 16 );
		bitpacker.ReadBits( e_out, 20 );
		bitpacker.ReadBits( f_out, 6 );
		bitpacker.ReadBits( g_out, 6 );
		bitpacker.ReadBits( h_out, 7 );
		CHECK( bitpacker.GetBits() == used_bits );
		CHECK( bitpacker.GetBytes() == used_bytes );
		CHECK( bitpacker.BitsRemaining() == total_bits - used_bits );

		CHECK( a == a_out );
		CHECK( b == b_out );
		CHECK( c == c_out );
		CHECK( d == d_out );
		CHECK( e == e_out );
		CHECK( f == f_out );
		CHECK( g == g_out );
		CHECK( h == h_out );
	}
}

SUITE( Stream )
{
	TEST( bits_required )
	{
		CHECK( Stream::BitsRequired( 0, 1 ) == 1 );
		CHECK( Stream::BitsRequired( 0, 3 ) == 2 );
		CHECK( Stream::BitsRequired( 0, 7 ) == 3 );
		CHECK( Stream::BitsRequired( 0, 15 ) == 4 );
		CHECK( Stream::BitsRequired( 0, 31 ) == 5 );
		CHECK( Stream::BitsRequired( 0, 63 ) == 6 );
		CHECK( Stream::BitsRequired( 0, 127 ) == 7 );
		CHECK( Stream::BitsRequired( 0, 255 ) == 8 );
		CHECK( Stream::BitsRequired( 0, 511 ) == 9 );
		CHECK( Stream::BitsRequired( 0, 1023 ) == 10 );
	}

	TEST( serialize_boolean )
	{
		unsigned char buffer[256];
		memset( buffer, 0, sizeof( buffer ) );

		bool a = false;
		bool b = true;
		bool c = false;
		bool d = false;
		bool e = true;
		bool f = false;
		bool g = true;
		bool h = true;

		const int total_bits = 256 * 8;
		const int used_bits = 8;

 		Stream stream( Stream::Write, buffer, sizeof(buffer) );
		CHECK( stream.SerializeBoolean( a ) );
		CHECK( stream.SerializeBoolean( b ) );
		CHECK( stream.SerializeBoolean( c ) );
		CHECK( stream.SerializeBoolean( d ) );
		CHECK( stream.SerializeBoolean( e ) );
		CHECK( stream.SerializeBoolean( f ) );
		CHECK( stream.SerializeBoolean( g ) );
		CHECK( stream.SerializeBoolean( h ) );
		CHECK( stream.GetBitsProcessed() == used_bits );
		CHECK( stream.GetBitsRemaining() == total_bits - used_bits );

		bool a_out = false;
		bool b_out = false;
		bool c_out = false;
		bool d_out = false;
		bool e_out = false;
		bool f_out = false;
		bool g_out = false;
		bool h_out = false;

		stream = Stream( Stream::Read, buffer, sizeof(buffer ) );
		CHECK( stream.SerializeBoolean( a_out ) );
		CHECK( stream.SerializeBoolean( b_out ) );
		CHECK( stream.SerializeBoolean( c_out ) );
		CHECK( stream.SerializeBoolean( d_out ) );
		CHECK( stream.SerializeBoolean( e_out ) );
		CHECK( stream.SerializeBoolean( f_out ) );
		CHECK( stream.SerializeBoolean( g_out ) );
		CHECK( stream.SerializeBoolean( h_out ) );
		CHECK( stream.GetBitsProcessed() == used_bits );
		CHECK( stream.GetBitsRemaining() == total_bits - used_bits );

		CHECK( a == a_out );
		CHECK( b == b_out );
		CHECK( c == c_out );
		CHECK( d == d_out );
		CHECK( e == e_out );
		CHECK( f == f_out );
		CHECK( g == g_out );
		CHECK( h == h_out );
	}

	TEST( serialize_byte )
	{
		unsigned char buffer[256];
		memset( buffer, 0, sizeof( buffer ) );

		unsigned char a = 123;
		unsigned char b = 1;
		unsigned char c = 10;
		unsigned char d = 50;
		unsigned char e = 2;
		unsigned char f = 68;
		unsigned char g = 190;
		unsigned char h = 210;

		const int total_bits = 256 * 8;
		const int used_bits = 7 + 1 + 4 + 6 + 2 + 7 + 8 + 8;

 		Stream stream( Stream::Write, buffer, sizeof(buffer) );
		CHECK( stream.SerializeByte( a, 0, a ) );
		CHECK( stream.SerializeByte( b, 0, b ) );
		CHECK( stream.SerializeByte( c, 0, c ) );
		CHECK( stream.SerializeByte( d, 0, d ) );
		CHECK( stream.SerializeByte( e, 0, e ) );
		CHECK( stream.SerializeByte( f, 0, f ) );
		CHECK( stream.SerializeByte( g, 0, g ) );
		CHECK( stream.SerializeByte( h, 0, h ) );
		CHECK( stream.GetBitsProcessed() == used_bits );
		CHECK( stream.GetBitsRemaining() == total_bits - used_bits );

		unsigned char a_out = 0xFF;
		unsigned char b_out = 0xFF;
		unsigned char c_out = 0xFF;
		unsigned char d_out = 0xFF;
		unsigned char e_out = 0xFF;
		unsigned char f_out = 0xFF;
		unsigned char g_out = 0xFF;
		unsigned char h_out = 0xFF;

		stream = Stream( Stream::Read, buffer, sizeof(buffer ) );
		CHECK( stream.SerializeByte( a_out, 0, a ) );
		CHECK( stream.SerializeByte( b_out, 0, b ) );
		CHECK( stream.SerializeByte( c_out, 0, c ) );
		CHECK( stream.SerializeByte( d_out, 0, d ) );
		CHECK( stream.SerializeByte( e_out, 0, e ) );
		CHECK( stream.SerializeByte( f_out, 0, f ) );
		CHECK( stream.SerializeByte( g_out, 0, g ) );
		CHECK( stream.SerializeByte( h_out, 0, h ) );
		CHECK( stream.GetBitsProcessed() == used_bits );
		CHECK( stream.GetBitsRemaining() == total_bits - used_bits );

		CHECK( a == a_out );
		CHECK( b == b_out );
		CHECK( c == c_out );
		CHECK( d == d_out );
		CHECK( e == e_out );
		CHECK( f == f_out );
		CHECK( g == g_out );
		CHECK( h == h_out );
	}

	TEST( serialize_short )
	{
		unsigned char buffer[256];
		memset( buffer, 0, sizeof( buffer ) );

		unsigned short a = 123;
		unsigned short b = 1;
		unsigned short c = 10004;
		unsigned short d = 50234;
		unsigned short e = 2;
		unsigned short f = 55;
		unsigned short g = 40;
		unsigned short h = 100;

		const int total_bits = 256 * 8;
		const int used_bits = 7 + 1 + 14 + 16 + 2 + 6 + 6 + 7;

 		Stream stream( Stream::Write, buffer, sizeof(buffer) );
		CHECK( stream.SerializeShort( a, 0, a ) );
		CHECK( stream.SerializeShort( b, 0, b ) );
		CHECK( stream.SerializeShort( c, 0, c ) );
		CHECK( stream.SerializeShort( d, 0, d ) );
		CHECK( stream.SerializeShort( e, 0, e ) );
		CHECK( stream.SerializeShort( f, 0, f ) );
		CHECK( stream.SerializeShort( g, 0, g ) );
		CHECK( stream.SerializeShort( h, 0, h ) );
		CHECK( stream.GetBitsProcessed() == used_bits );
		CHECK( stream.GetBitsRemaining() == total_bits - used_bits );

		unsigned short a_out = 0xFFFF;
		unsigned short b_out = 0xFFFF;
		unsigned short c_out = 0xFFFF;
		unsigned short d_out = 0xFFFF;
		unsigned short e_out = 0xFFFF;
		unsigned short f_out = 0xFFFF;
		unsigned short g_out = 0xFFFF;
		unsigned short h_out = 0xFFFF;

		stream = Stream( Stream::Read, buffer, sizeof(buffer ) );
		CHECK( stream.SerializeShort( a_out, 0, a ) );
		CHECK( stream.SerializeShort( b_out, 0, b ) );
		CHECK( stream.SerializeShort( c_out, 0, c ) );
		CHECK( stream.SerializeShort( d_out, 0, d ) );
		CHECK( stream.SerializeShort( e_out, 0, e ) );
		CHECK( stream.SerializeShort( f_out, 0, f ) );
		CHECK( stream.SerializeShort( g_out, 0, g ) );
		CHECK( stream.SerializeShort( h_out, 0, h ) );
		CHECK( stream.GetBitsProcessed() == used_bits );
		CHECK( stream.GetBitsRemaining() == total_bits - used_bits );

		CHECK( a == a_out );
		CHECK( b == b_out );
		CHECK( c == c_out );
		CHECK( d == d_out );
		CHECK( e == e_out );
		CHECK( f == f_out );
		CHECK( g == g_out );
		CHECK( h == h_out );
	}
	
	TEST( serialize_integer )
	{
		unsigned char buffer[256];
		memset( buffer, 0, sizeof( buffer ) );

		unsigned int a = 123;
		unsigned int b = 1;
		unsigned int c = 10004;
		unsigned int d = 50234;
		unsigned int e = 1020491;
		unsigned int f = 55;
		unsigned int g = 40;
		unsigned int h = 100;

		const int total_bits = 256 * 8;
		const int used_bits = 7 + 1 + 14 + 16 + 20 + 6 + 6 + 7;

 		Stream stream( Stream::Write, buffer, sizeof(buffer) );
		CHECK( stream.SerializeInteger( a, 0, a ) );
		CHECK( stream.SerializeInteger( b, 0, b ) );
		CHECK( stream.SerializeInteger( c, 0, c ) );
		CHECK( stream.SerializeInteger( d, 0, d ) );
		CHECK( stream.SerializeInteger( e, 0, e ) );
		CHECK( stream.SerializeInteger( f, 0, f ) );
		CHECK( stream.SerializeInteger( g, 0, g ) );
		CHECK( stream.SerializeInteger( h, 0, h ) );
		CHECK( stream.GetBitsProcessed() == used_bits );
		CHECK( stream.GetBitsRemaining() == total_bits - used_bits );

		unsigned int a_out = 0xFFFFFFFF;
		unsigned int b_out = 0xFFFFFFFF;
		unsigned int c_out = 0xFFFFFFFF;
		unsigned int d_out = 0xFFFFFFFF;
		unsigned int e_out = 0xFFFFFFFF;
		unsigned int f_out = 0xFFFFFFFF;
		unsigned int g_out = 0xFFFFFFFF;
		unsigned int h_out = 0xFFFFFFFF;

		stream = Stream( Stream::Read, buffer, sizeof(buffer ) );
		CHECK( stream.SerializeInteger( a_out, 0, a ) );
		CHECK( stream.SerializeInteger( b_out, 0, b ) );
		CHECK( stream.SerializeInteger( c_out, 0, c ) );
		CHECK( stream.SerializeInteger( d_out, 0, d ) );
		CHECK( stream.SerializeInteger( e_out, 0, e ) );
		CHECK( stream.SerializeInteger( f_out, 0, f ) );
		CHECK( stream.SerializeInteger( g_out, 0, g ) );
		CHECK( stream.SerializeInteger( h_out, 0, h ) );
		CHECK( stream.GetBitsProcessed() == used_bits );
		CHECK( stream.GetBitsRemaining() == total_bits - used_bits );

		CHECK( a == a_out );
		CHECK( b == b_out );
		CHECK( c == c_out );
		CHECK( d == d_out );
		CHECK( e == e_out );
		CHECK( f == f_out );
		CHECK( g == g_out );
		CHECK( h == h_out );
	}

	TEST( serialize_float )
	{
		unsigned char buffer[256];
		memset( buffer, 0, sizeof( buffer ) );

		float a = 12.3f;
		float b = 1.8753f;
 		float c = 10004.017231f;
		float d = 50234.01231f;
		float e = 1020491.5834f;
		float f = 55.0f;
		float g = 40.9f;
		float h = 100.001f;

		const int total_bits = 256 * 8;
		const int used_bits = 8 * 32;

 		Stream stream( Stream::Write, buffer, sizeof(buffer) );
		CHECK( stream.SerializeFloat( a ) );
		CHECK( stream.SerializeFloat( b ) );
		CHECK( stream.SerializeFloat( c ) );
		CHECK( stream.SerializeFloat( d ) );
		CHECK( stream.SerializeFloat( e ) );
		CHECK( stream.SerializeFloat( f ) );
		CHECK( stream.SerializeFloat( g ) );
		CHECK( stream.SerializeFloat( h ) );
		CHECK( stream.GetBitsProcessed() == used_bits );
		CHECK( stream.GetBitsRemaining() == total_bits - used_bits );

 		float a_out = 0.0f;
 		float b_out = 0.0f;
 		float c_out = 0.0f;
 		float d_out = 0.0f;
 		float e_out = 0.0f;
 		float f_out = 0.0f;
 		float g_out = 0.0f;
 		float h_out = 0.0f;

		stream = Stream( Stream::Read, buffer, sizeof(buffer ) );
		CHECK( stream.SerializeFloat( a_out ) );
		CHECK( stream.SerializeFloat( b_out ) );
		CHECK( stream.SerializeFloat( c_out ) );
		CHECK( stream.SerializeFloat( d_out ) );
		CHECK( stream.SerializeFloat( e_out ) );
		CHECK( stream.SerializeFloat( f_out ) );
		CHECK( stream.SerializeFloat( g_out ) );
		CHECK( stream.SerializeFloat( h_out ) );
		CHECK( stream.GetBitsProcessed() == used_bits );
		CHECK( stream.GetBitsRemaining() == total_bits - used_bits );

		CHECK( a == a_out );
		CHECK( b == b_out );
		CHECK( c == c_out );
		CHECK( d == d_out );
		CHECK( e == e_out );
		CHECK( f == f_out );
		CHECK( g == g_out );
		CHECK( h == h_out );
	}

	TEST( serialize_signed_byte )
	{
		unsigned char buffer[256];
		memset( buffer, 0, sizeof( buffer ) );

		signed char min = -100;
		signed char max = +100;

 		Stream stream( Stream::Write, buffer, sizeof(buffer) );
		for ( signed char i = min; i <= max; ++i )
			CHECK( stream.SerializeByte( i, min, max ) );

		stream = Stream( Stream::Read, buffer, sizeof(buffer) );
		for ( signed char i = min; i <= max; ++i )
		{
			signed char value = 0;
			CHECK( stream.SerializeByte( value, min, max ) );
			CHECK( value == i );
		}
	}

	TEST( serialize_signed_short )
	{
		unsigned char buffer[2048];
		memset( buffer, 0, sizeof( buffer ) );

		signed short min = -500;
		signed short max = +500;

 		Stream stream( Stream::Write, buffer, sizeof(buffer) );
		for ( signed short i = min; i <= max; ++i )
			CHECK( stream.SerializeShort( i, min, max ) );

		stream = Stream( Stream::Read, buffer, sizeof(buffer) );
		for ( signed short i = min; i <= max; ++i )
		{
			signed short value = 0;
			CHECK( stream.SerializeShort( value, min, max ) );
			CHECK( value == i );
		}
	}

	TEST( serialize_signed_int )
	{
		unsigned char buffer[2048];
		memset( buffer, 0, sizeof( buffer ) );

		signed int min = -100000;
		signed int max = +100000;

 		Stream stream( Stream::Write, buffer, sizeof(buffer) );
		for ( signed int i = min; i <= max; i += 1000 )
			CHECK( stream.SerializeInteger( i, min, max ) );

		stream = Stream( Stream::Read, buffer, sizeof(buffer) );
		for ( signed int i = min; i <= max; i += 1000 )
		{
			signed int value = 0;
			CHECK( stream.SerializeInteger( value, min, max ) );
			CHECK( value == i );
		}
	}

	TEST( checkpoint )
	{
		unsigned char buffer[256];
		memset( buffer, 0, sizeof( buffer ) );

		unsigned int a = 123;
		unsigned int b = 1;
		unsigned int c = 10004;

 		Stream stream( Stream::Write, buffer, sizeof(buffer) );
		CHECK( stream.Checkpoint() );
		CHECK( stream.SerializeInteger( a, 0, a ) );
		CHECK( stream.Checkpoint() );
		CHECK( stream.SerializeInteger( b, 0, b ) );
		CHECK( stream.Checkpoint() );
		CHECK( stream.SerializeInteger( c, 0, c ) );
		CHECK( stream.Checkpoint() );

		unsigned int a_out = 0xFFFFFFFF;
		unsigned int b_out = 0xFFFFFFFF;
		unsigned int c_out = 0xFFFFFFFF;

		stream = Stream( Stream::Read, buffer, sizeof(buffer ) );
		CHECK( stream.Checkpoint() );
		CHECK( stream.SerializeInteger( a_out, 0, a ) );
		CHECK( stream.Checkpoint() );
		CHECK( stream.SerializeInteger( b_out, 0, b ) );
		CHECK( stream.Checkpoint() );
		CHECK( stream.SerializeInteger( c_out, 0, c ) );
		CHECK( stream.Checkpoint() );

		CHECK( a == a_out );
		CHECK( b == b_out );
		CHECK( c == c_out );
	}

	TEST( journal )
	{
		unsigned char buffer[256];
		unsigned char journal[256];
		memset( buffer, 0, sizeof( buffer ) );
		memset( journal, 0, sizeof( journal ) );

		unsigned int a = 123;
		unsigned int b = 1;
		unsigned int c = 10004;

 		Stream stream( Stream::Write, buffer, sizeof(buffer), journal, sizeof(journal) );
		CHECK( stream.Checkpoint() );
		CHECK( stream.SerializeInteger( a, 0, a ) );
		CHECK( stream.Checkpoint() );
		CHECK( stream.SerializeInteger( b, 0, b ) );
		CHECK( stream.Checkpoint() );
		CHECK( stream.SerializeInteger( c, 0, c ) );
		CHECK( stream.Checkpoint() );

		unsigned int a_out = 0xFFFFFFFF;
		unsigned int b_out = 0xFFFFFFFF;
		unsigned int c_out = 0xFFFFFFFF;

		stream = Stream( Stream::Read, buffer, sizeof(buffer ), journal, sizeof(journal) );
		CHECK( stream.Checkpoint() );
		CHECK( stream.SerializeInteger( a_out, 0, a ) );
		CHECK( stream.Checkpoint() );
		CHECK( stream.SerializeInteger( b_out, 0, b ) );
		CHECK( stream.Checkpoint() );
		CHECK( stream.SerializeInteger( c_out, 0, c ) );
		CHECK( stream.Checkpoint() );

		CHECK( a == a_out );
		CHECK( b == b_out );
		CHECK( c == c_out );
	}
	
	TEST( stream_packet )
	{
		unsigned int ProtocolId = 0x12345678;
		
		unsigned char buffer[256];
		unsigned char journal[256];
		unsigned char packet[1024];
		memset( buffer, 0, sizeof( buffer ) );
		memset( journal, 0, sizeof( journal ) );
		memset( packet, 0, sizeof( packet ) );

		unsigned int a = 123;
		unsigned int b = 1;
		unsigned int c = 10004;

 		Stream stream( Stream::Write, buffer, sizeof(buffer), journal, sizeof(journal) );
		CHECK( stream.Checkpoint() );
		CHECK( stream.SerializeInteger( a, 0, a ) );
		CHECK( stream.Checkpoint() );
		CHECK( stream.SerializeInteger( b, 0, b ) );
		CHECK( stream.Checkpoint() );
		CHECK( stream.SerializeInteger( c, 0, c ) );
		CHECK( stream.Checkpoint() );
		
		int packetSize = sizeof( packet );
		BuildStreamPacket( stream, packet, packetSize, ProtocolId );
		
		CHECK( ReadStreamPacket( stream, packet, packetSize, ProtocolId ) );
		
		unsigned int a_out = 0xFFFFFFFF;
		unsigned int b_out = 0xFFFFFFFF;
		unsigned int c_out = 0xFFFFFFFF;

		CHECK( stream.Checkpoint() );
		CHECK( stream.SerializeInteger( a_out, 0, a ) );
		CHECK( stream.Checkpoint() );
		CHECK( stream.SerializeInteger( b_out, 0, b ) );
		CHECK( stream.Checkpoint() );
		CHECK( stream.SerializeInteger( c_out, 0, c ) );
		CHECK( stream.Checkpoint() );

		CHECK( a == a_out );
		CHECK( b == b_out );
		CHECK( c == c_out );
	}
}

// ------------------------------------------------------------------------------------------------------

SUITE( Connection )
{
	TEST( connection_connect )
	{
		const int ServerPort = 20000;
		const int ClientPort = 20001;
		const int ProtocolId = 0x11112222;
		const float DeltaTime = 0.001f;
		const float TimeOut = 1.0f;

		Connection client( ProtocolId, TimeOut );
		Connection server( ProtocolId, TimeOut );

		CHECK( client.Start( ClientPort ) );
		CHECK( server.Start( ServerPort ) );

		client.Connect( Address(127,0,0,1,ServerPort ) );
		server.Listen();

		while ( true )
		{
			if ( client.IsConnected() && server.IsConnected() )
				break;

			if ( !client.IsConnecting() && client.ConnectFailed() )
				break;

			unsigned char client_packet[] = "client to server";
			client.SendPacket( client_packet, sizeof( client_packet ) );

			unsigned char server_packet[] = "server to client";
			server.SendPacket( server_packet, sizeof( server_packet ) );

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}

			client.Update( DeltaTime );
			server.Update( DeltaTime );
		}

		CHECK( client.IsConnected() );
		CHECK( server.IsConnected() );
	}

	TEST( connection_connect_timeout )
	{
		const int ServerPort = 20000;
		const int ClientPort = 20001;
		const int ProtocolId = 0x11112222;
		const float DeltaTime = 0.001f;
		const float TimeOut = 0.1f;
	
		Connection client( ProtocolId, TimeOut );
	
		CHECK( client.Start( ClientPort ) );
	
		client.Connect( Address(127,0,0,1,ServerPort ) );
	
		while ( true )
		{
			if ( !client.IsConnecting() )
				break;
		
			unsigned char client_packet[] = "client to server";
			client.SendPacket( client_packet, sizeof( client_packet ) );

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}
		
			client.Update( DeltaTime );
		}
	
		CHECK( !client.IsConnected() );
		CHECK( client.ConnectFailed() );
	}

	TEST( connection_connect_busy )
	{
		const int ServerPort = 20000;
		const int ClientPort = 20001;
		const int ProtocolId = 0x11112222;
		const float DeltaTime = 0.001f;
		const float TimeOut = 1.0f;
	
		// connect client to server
	
		Connection client( ProtocolId, TimeOut );
		Connection server( ProtocolId, TimeOut );
	
		CHECK( client.Start( ClientPort ) );
		CHECK( server.Start( ServerPort ) );
	
		client.Connect( Address(127,0,0,1,ServerPort ) );
		server.Listen();
	
		while ( true )
		{
			if ( client.IsConnected() && server.IsConnected() )
				break;
			
			if ( !client.IsConnecting() && client.ConnectFailed() )
				break;
		
			unsigned char client_packet[] = "client to server";
			client.SendPacket( client_packet, sizeof( client_packet ) );

			unsigned char server_packet[] = "server to client";
			server.SendPacket( server_packet, sizeof( server_packet ) );
		
			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}
		
			client.Update( DeltaTime );
			server.Update( DeltaTime );
		}
	
		CHECK( client.IsConnected() );
		CHECK( server.IsConnected() );

		// attempt another connection, verify connect fails (busy)
	
		Connection busy( ProtocolId, TimeOut );
		CHECK( busy.Start( ClientPort + 1 ) );
		busy.Connect( Address(127,0,0,1,ServerPort ) );

		while ( true )
		{
			if ( !busy.IsConnecting() || busy.IsConnected() )
				break;
		
			unsigned char client_packet[] = "client to server";
			client.SendPacket( client_packet, sizeof( client_packet ) );

			unsigned char server_packet[] = "server to client";
			server.SendPacket( server_packet, sizeof( server_packet ) );
		
			unsigned char busy_packet[] = "i'm so busy!";
			busy.SendPacket( busy_packet, sizeof( busy_packet ) );
		
			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}
		
			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = busy.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}

			client.Update( DeltaTime );
			server.Update( DeltaTime );
			busy.Update( DeltaTime );
		}

		CHECK( client.IsConnected() );
		CHECK( server.IsConnected() );
		CHECK( !busy.IsConnected() );
		CHECK( busy.ConnectFailed() );
	}

	TEST( connection_reconnect )
	{
		const int ServerPort = 20000;
		const int ClientPort = 20001;
		const int ProtocolId = 0x11112222;
		const float DeltaTime = 0.001f;
		const float TimeOut = 1.0f;
	
		Connection client( ProtocolId, TimeOut );
		Connection server( ProtocolId, TimeOut );
	
		CHECK( client.Start( ClientPort ) );
		CHECK( server.Start( ServerPort ) );

		// connect client and server
	
		client.Connect( Address(127,0,0,1,ServerPort ) );
		server.Listen();
	
		while ( true )
		{
			if ( client.IsConnected() && server.IsConnected() )
				break;
			
			if ( !client.IsConnecting() && client.ConnectFailed() )
				break;
		
			unsigned char client_packet[] = "client to server";
			client.SendPacket( client_packet, sizeof( client_packet ) );

			unsigned char server_packet[] = "server to client";
			server.SendPacket( server_packet, sizeof( server_packet ) );
		
			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}
		
			client.Update( DeltaTime );
			server.Update( DeltaTime );
		}
	
		CHECK( client.IsConnected() );
		CHECK( server.IsConnected() );

		// let connection timeout

		while ( client.IsConnected() || server.IsConnected() )
		{
			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}

			client.Update( DeltaTime );
			server.Update( DeltaTime );
		}
	
		CHECK( !client.IsConnected() );
		CHECK( !server.IsConnected() );

		// reconnect client

		client.Connect( Address(127,0,0,1,ServerPort ) );
	
		while ( true )
		{
			if ( client.IsConnected() && server.IsConnected() )
				break;
			
			if ( !client.IsConnecting() && client.ConnectFailed() )
				break;
		
			unsigned char client_packet[] = "client to server";
			client.SendPacket( client_packet, sizeof( client_packet ) );

			unsigned char server_packet[] = "server to client";
			server.SendPacket( server_packet, sizeof( server_packet ) );
		
			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}
		
			client.Update( DeltaTime );
			server.Update( DeltaTime );
		}
	
		CHECK( client.IsConnected() );
		CHECK( server.IsConnected() );
	}

	TEST( connection_payload )
	{
		const int ServerPort = 20000;
		const int ClientPort = 20001;
		const int ProtocolId = 0x11112222;
		const float DeltaTime = 0.001f;
		const float TimeOut = 0.1f;
	
		Connection client( ProtocolId, TimeOut );
		Connection server( ProtocolId, TimeOut );
	
		CHECK( client.Start( ClientPort ) );
		CHECK( server.Start( ServerPort ) );
	
		client.Connect( Address(127,0,0,1,ServerPort ) );
		server.Listen();
	
		while ( true )
		{
			if ( client.IsConnected() && server.IsConnected() )
				break;
			
			if ( !client.IsConnecting() && client.ConnectFailed() )
				break;
		
			unsigned char client_packet[] = "client to server";
			client.SendPacket( client_packet, sizeof( client_packet ) );

			unsigned char server_packet[] = "server to client";
			server.SendPacket( server_packet, sizeof( server_packet ) );
		
			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
				CHECK( strcmp( (const char*) packet, "server to client" ) == 0 );
			}

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
				CHECK( strcmp( (const char*) packet, "client to server" ) == 0 );
			}
		
			client.Update( DeltaTime );
			server.Update( DeltaTime );
		}
	
		CHECK( client.IsConnected() );
		CHECK( server.IsConnected() );
	}
}

// ------------------------------------------------------------------------------------------------------

SUITE( PacketQueue )
{
	const unsigned int MaximumSequence = 255;

	TEST( insert_back )
	{
		net::PacketQueue packetQueue;
		for ( int i = 0; i < 100; ++i )
		{
			PacketData data;
			data.sequence = i;
			packetQueue.insert_sorted( data, MaximumSequence );
			packetQueue.verify_sorted( MaximumSequence );
		}
	}
	
	TEST( insert_front )
	{		
		net::PacketQueue packetQueue;
		for ( int i = 100; i < 0; ++i )
		{
			PacketData data;
			data.sequence = i;
			packetQueue.insert_sorted( data, MaximumSequence );
			packetQueue.verify_sorted( MaximumSequence );
		}
	}

	TEST( insert_random )
	{
		net::PacketQueue packetQueue;
		for ( int i = 100; i < 0; ++i )
		{
			PacketData data;
			data.sequence = rand() & 0xFF;
			packetQueue.insert_sorted( data, MaximumSequence );
			packetQueue.verify_sorted( MaximumSequence );
		}
	}

	TEST( insert_wrap_around )
	{
		net::PacketQueue packetQueue;
		for ( int i = 200; i <= 255; ++i )
		{
			PacketData data;
			data.sequence = i;
			packetQueue.insert_sorted( data, MaximumSequence );
			packetQueue.verify_sorted( MaximumSequence );
		}
		for ( int i = 0; i <= 50; ++i )
		{
			PacketData data;
			data.sequence = i;
			packetQueue.insert_sorted( data, MaximumSequence );
			packetQueue.verify_sorted( MaximumSequence );
		}
	}
}

// ------------------------------------------------------------------------------------------------------

SUITE( Reliability )
{
	const int MaximumSequence = 255;

	TEST( bit_index_for_sequence )
	{
		CHECK( ReliabilitySystem::bit_index_for_sequence( 99, 100, MaximumSequence ) == 0 );
		CHECK( ReliabilitySystem::bit_index_for_sequence( 90, 100, MaximumSequence ) == 9 );
		CHECK( ReliabilitySystem::bit_index_for_sequence( 0, 1, MaximumSequence ) == 0 );
		CHECK( ReliabilitySystem::bit_index_for_sequence( 255, 0, MaximumSequence ) == 0 );
		CHECK( ReliabilitySystem::bit_index_for_sequence( 255, 1, MaximumSequence ) == 1 );
		CHECK( ReliabilitySystem::bit_index_for_sequence( 254, 1, MaximumSequence ) == 2 );
		CHECK( ReliabilitySystem::bit_index_for_sequence( 254, 2, MaximumSequence ) == 3 );
	}
	
	TEST( generate_ack_bits )
	{
		net::PacketQueue packetQueue;
		for ( int i = 0; i < 32; ++i )
		{
			PacketData data;
			data.sequence = i;
			packetQueue.insert_sorted( data, MaximumSequence );
			packetQueue.verify_sorted( MaximumSequence );
		}
		CHECK( ReliabilitySystem::generate_ack_bits( 32, packetQueue, MaximumSequence ) == 0xFFFFFFFF );
		CHECK( ReliabilitySystem::generate_ack_bits( 31, packetQueue, MaximumSequence ) == 0x7FFFFFFF );
		CHECK( ReliabilitySystem::generate_ack_bits( 33, packetQueue, MaximumSequence ) == 0xFFFFFFFE );
		CHECK( ReliabilitySystem::generate_ack_bits( 16, packetQueue, MaximumSequence ) == 0x0000FFFF );
		CHECK( ReliabilitySystem::generate_ack_bits( 48, packetQueue, MaximumSequence ) == 0xFFFF0000 );
	}
	
	TEST( generate_ack_bits_with_wrap )
	{
		net::PacketQueue packetQueue;
		for ( int i = 255 - 31; i <= 255; ++i )
		{
			PacketData data;
			data.sequence = i;
			packetQueue.insert_sorted( data, MaximumSequence );
			packetQueue.verify_sorted( MaximumSequence );
		}
		CHECK( packetQueue.size() == 32 );
		CHECK( ReliabilitySystem::generate_ack_bits( 0, packetQueue, MaximumSequence ) == 0xFFFFFFFF );
		CHECK( ReliabilitySystem::generate_ack_bits( 255, packetQueue, MaximumSequence ) == 0x7FFFFFFF );
		CHECK( ReliabilitySystem::generate_ack_bits( 1, packetQueue, MaximumSequence ) == 0xFFFFFFFE );
		CHECK( ReliabilitySystem::generate_ack_bits( 240, packetQueue, MaximumSequence ) == 0x0000FFFF );
		CHECK( ReliabilitySystem::generate_ack_bits( 16, packetQueue, MaximumSequence ) == 0xFFFF0000 );
	}
	
	TEST( process_ack_1 )
	{
		net::PacketQueue pendingAckQueue;
		for ( int i = 0; i < 33; ++i )
		{
			PacketData data;
			data.sequence = i;
			data.time = 0.0f;
			pendingAckQueue.insert_sorted( data, MaximumSequence );
			pendingAckQueue.verify_sorted( MaximumSequence );
		}
		net::PacketQueue ackedQueue;
		std::vector<unsigned int> acks;
		float rtt = 0.0f;
		unsigned int acked_packets = 0;
		ReliabilitySystem::process_ack( 32, 0xFFFFFFFF, pendingAckQueue, ackedQueue, acks, acked_packets, rtt, MaximumSequence );
		CHECK( acks.size() == 33 );
		CHECK( acked_packets == 33 );
		CHECK( ackedQueue.size() == 33 );
		CHECK( pendingAckQueue.size() == 0 );
		ackedQueue.verify_sorted( MaximumSequence );
		for ( unsigned int i = 0; i < acks.size(); ++i )
			CHECK( acks[i] == i );
		unsigned int i = 0;
		for ( net::PacketQueue::iterator itor = ackedQueue.begin(); itor != ackedQueue.end(); ++itor, ++i )
			CHECK( itor->sequence == i );
	}
	
	TEST( process_ack_2 )
	{
		net::PacketQueue pendingAckQueue;
		for ( int i = 0; i < 33; ++i )
		{
			PacketData data;
			data.sequence = i;
			data.time = 0.0f;
			pendingAckQueue.insert_sorted( data, MaximumSequence );
			pendingAckQueue.verify_sorted( MaximumSequence );
		}
		net::PacketQueue ackedQueue;
		std::vector<unsigned int> acks;
		float rtt = 0.0f;
		unsigned int acked_packets = 0;
		ReliabilitySystem::process_ack( 32, 0x0000FFFF, pendingAckQueue, ackedQueue, acks, acked_packets, rtt, MaximumSequence );
		CHECK( acks.size() == 17 );
		CHECK( acked_packets == 17 );
		CHECK( ackedQueue.size() == 17 );
		CHECK( pendingAckQueue.size() == 33 - 17 );
		ackedQueue.verify_sorted( MaximumSequence );
		unsigned int i = 0;
		for ( net::PacketQueue::iterator itor = pendingAckQueue.begin(); itor != pendingAckQueue.end(); ++itor, ++i )
			CHECK( itor->sequence == i );
		i = 0;
		for ( net::PacketQueue::iterator itor = ackedQueue.begin(); itor != ackedQueue.end(); ++itor, ++i )
			CHECK( itor->sequence == i + 16 );
		for ( unsigned int i = 0; i < acks.size(); ++i )
			CHECK( acks[i] == i + 16 );
	}

	TEST( process_ack_3 )
	{
		net::PacketQueue pendingAckQueue;
		for ( int i = 0; i < 32; ++i )
		{
			PacketData data;
			data.sequence = i;
			data.time = 0.0f;
			pendingAckQueue.insert_sorted( data, MaximumSequence );
			pendingAckQueue.verify_sorted( MaximumSequence );
		}
		net::PacketQueue ackedQueue;
		std::vector<unsigned int> acks;
		float rtt = 0.0f;
		unsigned int acked_packets = 0;
		ReliabilitySystem::process_ack( 48, 0xFFFF0000, pendingAckQueue, ackedQueue, acks, acked_packets, rtt, MaximumSequence );
		CHECK( acks.size() == 16 );
		CHECK( acked_packets == 16 );
		CHECK( ackedQueue.size() == 16 );
		CHECK( pendingAckQueue.size() == 16 );
		ackedQueue.verify_sorted( MaximumSequence );
		unsigned int i = 0;
		for ( net::PacketQueue::iterator itor = pendingAckQueue.begin(); itor != pendingAckQueue.end(); ++itor, ++i )
			CHECK( itor->sequence == i );
		i = 0;
		for ( net::PacketQueue::iterator itor = ackedQueue.begin(); itor != ackedQueue.end(); ++itor, ++i )
			CHECK( itor->sequence == i + 16 );
		for ( unsigned int i = 0; i < acks.size(); ++i )
			CHECK( acks[i] == i + 16 );
	}
	
	TEST( process_ack_wrap_around_1 )
	{
		net::PacketQueue pendingAckQueue;
		for ( int i = 255 - 31; i <= 256; ++i )
		{
			PacketData data;
			data.sequence = i & 0xFF;
			data.time = 0.0f;
			pendingAckQueue.insert_sorted( data, MaximumSequence );
			pendingAckQueue.verify_sorted( MaximumSequence );
		}
		CHECK( pendingAckQueue.size() == 33 );
		net::PacketQueue ackedQueue;
		std::vector<unsigned int> acks;
		float rtt = 0.0f;
		unsigned int acked_packets = 0;
		ReliabilitySystem::process_ack( 0, 0xFFFFFFFF, pendingAckQueue, ackedQueue, acks, acked_packets, rtt, MaximumSequence );
		CHECK( acks.size() == 33 );
		CHECK( acked_packets == 33 );
		CHECK( ackedQueue.size() == 33 );
		CHECK( pendingAckQueue.size() == 0 );
		ackedQueue.verify_sorted( MaximumSequence );
		for ( unsigned int i = 0; i < acks.size(); ++i )
			CHECK( acks[i] == ( (i+255-31) & 0xFF ) );
		unsigned int i = 0;
		for ( net::PacketQueue::iterator itor = ackedQueue.begin(); itor != ackedQueue.end(); ++itor, ++i )
			CHECK( itor->sequence == ( (i+255-31) & 0xFF ) );
	}
	
	TEST( process_ack_wrap_around_2 )
	{
		net::PacketQueue pendingAckQueue;
		for ( int i = 255 - 31; i <= 256; ++i )
		{
			PacketData data;
			data.sequence = i & 0xFF;
			data.time = 0.0f;
			pendingAckQueue.insert_sorted( data, MaximumSequence );
			pendingAckQueue.verify_sorted( MaximumSequence );
		}
		CHECK( pendingAckQueue.size() == 33 );
		net::PacketQueue ackedQueue;
		std::vector<unsigned int> acks;
		float rtt = 0.0f;
		unsigned int acked_packets = 0;
		ReliabilitySystem::process_ack( 0, 0x0000FFFF, pendingAckQueue, ackedQueue, acks, acked_packets, rtt, MaximumSequence );
		CHECK( acks.size() == 17 );
		CHECK( acked_packets == 17 );
		CHECK( ackedQueue.size() == 17 );
		CHECK( pendingAckQueue.size() == 33 - 17 );
		ackedQueue.verify_sorted( MaximumSequence );
		for ( unsigned int i = 0; i < acks.size(); ++i )
			CHECK( acks[i] == ( (i+255-15) & 0xFF ) );
		unsigned int i = 0;
		for ( net::PacketQueue::iterator itor = pendingAckQueue.begin(); itor != pendingAckQueue.end(); ++itor, ++i )
			CHECK( itor->sequence == i + 255 - 31 );
		i = 0;
		for ( net::PacketQueue::iterator itor = ackedQueue.begin(); itor != ackedQueue.end(); ++itor, ++i )
			CHECK( itor->sequence == ( (i+255-15) & 0xFF ) );
	}

	TEST( process_ack_wrap_around_3 )
	{
		net::PacketQueue pendingAckQueue;
		for ( int i = 255 - 31; i <= 255; ++i )
		{
			PacketData data;
			data.sequence = i & 0xFF;
			data.time = 0.0f;
			pendingAckQueue.insert_sorted( data, MaximumSequence );
			pendingAckQueue.verify_sorted( MaximumSequence );
		}
		CHECK( pendingAckQueue.size() == 32 );
		net::PacketQueue ackedQueue;
		std::vector<unsigned int> acks;
		float rtt = 0.0f;
		unsigned int acked_packets = 0;
		ReliabilitySystem::process_ack( 16, 0xFFFF0000, pendingAckQueue, ackedQueue, acks, acked_packets, rtt, MaximumSequence );
		CHECK( acks.size() == 16 );
		CHECK( acked_packets == 16 );
		CHECK( ackedQueue.size() == 16 );
		CHECK( pendingAckQueue.size() == 16 );
		ackedQueue.verify_sorted( MaximumSequence );
		for ( unsigned int i = 0; i < acks.size(); ++i )
			CHECK( acks[i] == ( (i+255-15) & 0xFF ) );
		unsigned int i = 0;
		for ( net::PacketQueue::iterator itor = pendingAckQueue.begin(); itor != pendingAckQueue.end(); ++itor, ++i )
			CHECK( itor->sequence == i + 255 - 31 );
		i = 0;
		for ( net::PacketQueue::iterator itor = ackedQueue.begin(); itor != ackedQueue.end(); ++itor, ++i )
			CHECK( itor->sequence == ( (i+255-15) & 0xFF ) );
	}
}

// ------------------------------------------------------------------------------------------------------

SUITE( ReliableConnection )
{
	TEST( reliable_connection_connect )
	{
		const int ServerPort = 20000;
		const int ClientPort = 20001;
		const int ProtocolId = 0x11112222;
		const float DeltaTime = 0.001f;
		const float TimeOut = 1.0f;

		ReliableConnection client( ProtocolId, TimeOut );
		ReliableConnection server( ProtocolId, TimeOut );

		CHECK( client.Start( ClientPort ) );
		CHECK( server.Start( ServerPort ) );

		client.Connect( Address(127,0,0,1,ServerPort ) );
		server.Listen();

		while ( true )
		{
			if ( client.IsConnected() && server.IsConnected() )
				break;

			if ( !client.IsConnecting() && client.ConnectFailed() )
				break;

			unsigned char client_packet[] = "client to server";
			client.SendPacket( client_packet, sizeof( client_packet ) );

			unsigned char server_packet[] = "server to client";
			server.SendPacket( server_packet, sizeof( server_packet ) );

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}

			client.Update( DeltaTime );
			server.Update( DeltaTime );
		}

		CHECK( client.IsConnected() );
		CHECK( server.IsConnected() );
	}
	
	TEST( reliable_connection_timeout )
	{
		const int ServerPort = 20000;
		const int ClientPort = 20001;
		const int ProtocolId = 0x11112222;
		const float DeltaTime = 0.001f;
		const float TimeOut = 0.1f;

		ReliableConnection client( ProtocolId, TimeOut );

		CHECK( client.Start( ClientPort ) );

		client.Connect( Address(127,0,0,1,ServerPort ) );

		while ( true )
		{
			if ( !client.IsConnecting() )
				break;

			unsigned char client_packet[] = "client to server";
			client.SendPacket( client_packet, sizeof( client_packet ) );

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}

			client.Update( DeltaTime );
		}

		CHECK( !client.IsConnected() );
		CHECK( client.ConnectFailed() );
	}
	
	TEST( reliable_connection_connect_busy )
	{
		const int ServerPort = 20000;
		const int ClientPort = 20001;
		const int ProtocolId = 0x11112222;
		const float DeltaTime = 0.001f;
		const float TimeOut = 0.1f;

		// connect client to server

		ReliableConnection client( ProtocolId, TimeOut );
		ReliableConnection server( ProtocolId, TimeOut );

		CHECK( client.Start( ClientPort ) );
		CHECK( server.Start( ServerPort ) );

		client.Connect( Address(127,0,0,1,ServerPort ) );
		server.Listen();

		while ( true )
		{
			if ( client.IsConnected() && server.IsConnected() )
				break;

			if ( !client.IsConnecting() && client.ConnectFailed() )
				break;

			unsigned char client_packet[] = "client to server";
			client.SendPacket( client_packet, sizeof( client_packet ) );

			unsigned char server_packet[] = "server to client";
			server.SendPacket( server_packet, sizeof( server_packet ) );

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}

			client.Update( DeltaTime );
			server.Update( DeltaTime );
		}

		CHECK( client.IsConnected() );
		CHECK( server.IsConnected() );

		// attempt another connection, verify connect fails (busy)

		ReliableConnection busy( ProtocolId, TimeOut );
		CHECK( busy.Start( ClientPort + 1 ) );
		busy.Connect( Address(127,0,0,1,ServerPort ) );

		while ( true )
		{
			if ( !busy.IsConnecting() || busy.IsConnected() )
				break;

			unsigned char client_packet[] = "client to server";
			client.SendPacket( client_packet, sizeof( client_packet ) );

			unsigned char server_packet[] = "server to client";
			server.SendPacket( server_packet, sizeof( server_packet ) );

			unsigned char busy_packet[] = "i'm so busy!";
			busy.SendPacket( busy_packet, sizeof( busy_packet ) );

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = busy.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}

			client.Update( DeltaTime );
			server.Update( DeltaTime );
			busy.Update( DeltaTime );
		}

		CHECK( client.IsConnected() );
		CHECK( server.IsConnected() );
		CHECK( !busy.IsConnected() );
		CHECK( busy.ConnectFailed() );
	}

	TEST( reliable_connection_reconnect )
	{
		const int ServerPort = 20000;
		const int ClientPort = 20001;
		const int ProtocolId = 0x11112222;
		const float DeltaTime = 0.001f;
		const float TimeOut = 0.1f;

		ReliableConnection client( ProtocolId, TimeOut );
		ReliableConnection server( ProtocolId, TimeOut );

		CHECK( client.Start( ClientPort ) );
		CHECK( server.Start( ServerPort ) );

		// connect client and server

		client.Connect( Address(127,0,0,1,ServerPort ) );
		server.Listen();

		while ( true )
		{
			if ( client.IsConnected() && server.IsConnected() )
				break;

			if ( !client.IsConnecting() && client.ConnectFailed() )
				break;

			unsigned char client_packet[] = "client to server";
			client.SendPacket( client_packet, sizeof( client_packet ) );

			unsigned char server_packet[] = "server to client";
			server.SendPacket( server_packet, sizeof( server_packet ) );

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}

			client.Update( DeltaTime );
			server.Update( DeltaTime );
		}

		CHECK( client.IsConnected() );
		CHECK( server.IsConnected() );

		// let connection timeout

		while ( client.IsConnected() || server.IsConnected() )
		{
			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}

			client.Update( DeltaTime );
			server.Update( DeltaTime );
		}

		CHECK( !client.IsConnected() );
		CHECK( !server.IsConnected() );

		// reconnect client

		client.Connect( Address(127,0,0,1,ServerPort ) );

		while ( true )
		{
			if ( client.IsConnected() && server.IsConnected() )
				break;

			if ( !client.IsConnecting() && client.ConnectFailed() )
				break;

			unsigned char client_packet[] = "client to server";
			client.SendPacket( client_packet, sizeof( client_packet ) );

			unsigned char server_packet[] = "server to client";
			server.SendPacket( server_packet, sizeof( server_packet ) );

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
			}

			client.Update( DeltaTime );
			server.Update( DeltaTime );
		}

		CHECK( client.IsConnected() );
		CHECK( server.IsConnected() );
	}

	TEST( reliable_connection_payload )
	{
		const int ServerPort = 20000;
		const int ClientPort = 20001;
		const int ProtocolId = 0x11112222;
		const float DeltaTime = 0.001f;
		const float TimeOut = 0.1f;

		ReliableConnection client( ProtocolId, TimeOut );
		ReliableConnection server( ProtocolId, TimeOut );

		CHECK( client.Start( ClientPort ) );
		CHECK( server.Start( ServerPort ) );

		client.Connect( Address(127,0,0,1,ServerPort ) );
		server.Listen();

		while ( true )
		{
			if ( client.IsConnected() && server.IsConnected() )
				break;

			if ( !client.IsConnecting() && client.ConnectFailed() )
				break;

			unsigned char client_packet[] = "client to server";
			client.SendPacket( client_packet, sizeof( client_packet ) );

			unsigned char server_packet[] = "server to client";
			server.SendPacket( server_packet, sizeof( server_packet ) );

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
				CHECK( strcmp( (const char*) packet, "server to client" ) == 0 );
			}

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
				CHECK( strcmp( (const char*) packet, "client to server" ) == 0 );
			}

			client.Update( DeltaTime );
			server.Update( DeltaTime );
		}

		CHECK( client.IsConnected() );
		CHECK( server.IsConnected() );
	}
	
	TEST( reliable_connection_acks )
	{
		const int ServerPort = 20000;
		const int ClientPort = 20001;
		const int ProtocolId = 0x11112222;
		const float DeltaTime = 0.001f;
		const float TimeOut = 0.1f;
		const unsigned int PacketCount = 100;

		ReliableConnection client( ProtocolId, TimeOut );
		ReliableConnection server( ProtocolId, TimeOut );

		CHECK( client.Start( ClientPort ) );
		CHECK( server.Start( ServerPort ) );

		client.Connect( Address(127,0,0,1,ServerPort ) );
		server.Listen();

		bool clientAckedPackets[PacketCount];
	 	bool serverAckedPackets[PacketCount];
		for ( unsigned int i = 0; i < PacketCount; ++i )
		{
			clientAckedPackets[i] = false;
			serverAckedPackets[i] = false;
		}

		bool allPacketsAcked = false;

		while ( true )
		{
			if ( !client.IsConnecting() && client.ConnectFailed() )
				break;

			if ( allPacketsAcked )
				break;

			unsigned char packet[256];
			for ( unsigned int i = 0; i < sizeof(packet); ++i )
				packet[i] = (unsigned char) i;

			server.SendPacket( packet, sizeof(packet) );
			client.SendPacket( packet, sizeof(packet) );

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
				CHECK( bytes_read == sizeof(packet) );
				for ( unsigned int i = 0; i < sizeof(packet); ++i )
					CHECK( packet[i] == (unsigned char) i );
			}

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
				CHECK( bytes_read == sizeof(packet) );
				for ( unsigned int i = 0; i < sizeof(packet); ++i )
					CHECK( packet[i] == (unsigned char) i );
			}

			int ack_count = 0;
			unsigned int * acks = NULL;
			client.GetReliabilitySystem().GetAcks( &acks, ack_count );
			CHECK( ack_count == 0 || ack_count != 0 && acks );
			for ( int i = 0; i < ack_count; ++i )
			{
				unsigned int ack = acks[i];
				if ( ack < PacketCount )
				{
					CHECK( clientAckedPackets[ack] == false );
					clientAckedPackets[ack] = true;
				}
			}

			server.GetReliabilitySystem().GetAcks( &acks, ack_count );
			CHECK( ack_count == 0 || ack_count != 0 && acks );
			for ( int i = 0; i < ack_count; ++i )
			{
				unsigned int ack = acks[i];
				if ( ack < PacketCount )
				{
					CHECK( serverAckedPackets[ack] == false );
					serverAckedPackets[ack] = true;
				}
			}

			unsigned int clientAckCount = 0;
			unsigned int serverAckCount = 0;
			for ( unsigned int i = 0; i < PacketCount; ++i )
			{
	 			clientAckCount += clientAckedPackets[i];
	 			serverAckCount += serverAckedPackets[i];
			}
			allPacketsAcked = clientAckCount == PacketCount && serverAckCount == PacketCount;

			client.Update( DeltaTime );
			server.Update( DeltaTime );
		}

		CHECK( client.IsConnected() );
		CHECK( server.IsConnected() );
	}

	TEST( reliable_connection_ack_bits )
	{
		const int ServerPort = 20000;
		const int ClientPort = 20001;
		const int ProtocolId = 0x11112222;
		const float DeltaTime = 0.001f;
		const float TimeOut = 0.1f;
		const unsigned int PacketCount = 100;

		ReliableConnection client( ProtocolId, TimeOut );
		ReliableConnection server( ProtocolId, TimeOut );

		CHECK( client.Start( ClientPort ) );
		CHECK( server.Start( ServerPort ) );

		client.Connect( Address(127,0,0,1,ServerPort ) );
		server.Listen();

		bool clientAckedPackets[PacketCount];
		bool serverAckedPackets[PacketCount];
		for ( unsigned int i = 0; i < PacketCount; ++i )
		{
			clientAckedPackets[i] = false;
			serverAckedPackets[i] = false;
		}

		bool allPacketsAcked = false;

		while ( true )
		{
			if ( !client.IsConnecting() && client.ConnectFailed() )
				break;

			if ( allPacketsAcked )
				break;

			unsigned char packet[256];
			for ( unsigned int i = 0; i < sizeof(packet); ++i )
				packet[i] = (unsigned char) i;

			for ( int i = 0; i < 10; ++i )
			{
				client.SendPacket( packet, sizeof(packet) );

				while ( true )
				{
					unsigned char packet[256];
					int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
					if ( bytes_read == 0 )
						break;
					CHECK( bytes_read == sizeof(packet) );
					for ( unsigned int i = 0; i < sizeof(packet); ++i )
						CHECK( packet[i] == (unsigned char) i );
				}

				int ack_count = 0;
				unsigned int * acks = NULL;
				client.GetReliabilitySystem().GetAcks( &acks, ack_count );
				CHECK( ack_count == 0 || ack_count != 0 && acks );
				for ( int i = 0; i < ack_count; ++i )
				{
					unsigned int ack = acks[i];
					if ( ack < PacketCount )
					{
						CHECK( !clientAckedPackets[ack] );
						clientAckedPackets[ack] = true;
					}
				}

				client.Update( DeltaTime * 0.1f );
			}

			server.SendPacket( packet, sizeof(packet) );

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
				CHECK( bytes_read == sizeof(packet) );
				for ( unsigned int i = 0; i < sizeof(packet); ++i )
					CHECK( packet[i] == (unsigned char) i );
			}

			int ack_count = 0;
			unsigned int * acks = NULL;
			server.GetReliabilitySystem().GetAcks( &acks, ack_count );
			CHECK( ack_count == 0 || ack_count != 0 && acks );
			for ( int i = 0; i < ack_count; ++i )
			{
				unsigned int ack = acks[i];
				if ( ack < PacketCount )
				{
					CHECK( !serverAckedPackets[ack] );
					serverAckedPackets[ack] = true;
				}
			}

			unsigned int clientAckCount = 0;
			unsigned int serverAckCount = 0;
			for ( unsigned int i = 0; i < PacketCount; ++i )
			{
				if ( clientAckedPackets[i] )
					clientAckCount++;
				if ( serverAckedPackets[i] )
					serverAckCount++;
			}
			allPacketsAcked = clientAckCount == PacketCount && serverAckCount == PacketCount;

			server.Update( DeltaTime );
		}

		CHECK( client.IsConnected() );
		CHECK( server.IsConnected() );
	}
	
	TEST( reliable_connection_packet_loss )
	{
		const int ServerPort = 20000;
		const int ClientPort = 20001;
		const int ProtocolId = 0x11112222;
		const float DeltaTime = 0.001f;
		const float TimeOut = 0.1f;
		const unsigned int PacketCount = 100;

		ReliableConnection client( ProtocolId, TimeOut );
		ReliableConnection server( ProtocolId, TimeOut );

		client.SetPacketLossMask( 1 );
		server.SetPacketLossMask( 1 );

		CHECK( client.Start( ClientPort ) );
		CHECK( server.Start( ServerPort ) );

		client.Connect( Address(127,0,0,1,ServerPort ) );
		server.Listen();

		bool clientAckedPackets[PacketCount];
		bool serverAckedPackets[PacketCount];
		for ( unsigned int i = 0; i < PacketCount; ++i )
		{
			clientAckedPackets[i] = false;
			serverAckedPackets[i] = false;
		}

		bool allPacketsAcked = false;

		while ( true )
		{
			if ( !client.IsConnecting() && client.ConnectFailed() )
				break;

			if ( allPacketsAcked )
				break;

			unsigned char packet[256];
			for ( unsigned int i = 0; i < sizeof(packet); ++i )
				packet[i] = (unsigned char) i;

			for ( int i = 0; i < 10; ++i )
			{
				client.SendPacket( packet, sizeof(packet) );

				while ( true )
				{
					unsigned char packet[256];
					int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
					if ( bytes_read == 0 )
						break;
					CHECK( bytes_read == sizeof(packet) );
					for ( unsigned int i = 0; i < sizeof(packet); ++i )
						CHECK( packet[i] == (unsigned char) i );
				}

				int ack_count = 0;
				unsigned int * acks = NULL;
				client.GetReliabilitySystem().GetAcks( &acks, ack_count );
				CHECK( ack_count == 0 || ack_count != 0 && acks );
				for ( int i = 0; i < ack_count; ++i )
				{
					unsigned int ack = acks[i];
					if ( ack < PacketCount )
					{
						CHECK( !clientAckedPackets[ack] );
						CHECK( ( ack & 1 ) == 0 );
						clientAckedPackets[ack] = true;
					}
				}

				client.Update( DeltaTime * 0.1f );
			}

			server.SendPacket( packet, sizeof(packet) );

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
				CHECK( bytes_read == sizeof(packet) );
				for ( unsigned int i = 0; i < sizeof(packet); ++i )
					CHECK( packet[i] == (unsigned char) i );
			}

			int ack_count = 0;
			unsigned int * acks = NULL;
			server.GetReliabilitySystem().GetAcks( &acks, ack_count );
			CHECK( ack_count == 0 || ack_count != 0 && acks );
			for ( int i = 0; i < ack_count; ++i )
			{
				unsigned int ack = acks[i];
				if ( ack < PacketCount )
				{
					CHECK( !serverAckedPackets[ack] );
					CHECK( ( ack & 1 ) == 0 );
					serverAckedPackets[ack] = true;
				}
			}

			unsigned int clientAckCount = 0;
			unsigned int serverAckCount = 0;
			for ( unsigned int i = 0; i < PacketCount; ++i )
			{
				if ( ( i & 1 ) != 0 )
				{
					CHECK( clientAckedPackets[i] == false );
					CHECK( serverAckedPackets[i] == false );
				}
				if ( clientAckedPackets[i] )
					clientAckCount++;
				if ( serverAckedPackets[i] )
					serverAckCount++;
			}
			allPacketsAcked = clientAckCount == PacketCount / 2 && serverAckCount == PacketCount / 2;

			server.Update( DeltaTime );
		}

		CHECK( client.IsConnected() );
		CHECK( server.IsConnected() );
	}

	TEST( reliable_connection_sequence_wrap_around )
	{
		const int ServerPort = 20000;
		const int ClientPort = 20001;
		const int ProtocolId = 0x11112222;
		const float DeltaTime = 0.05f;
		const float TimeOut = 1000.0f;
		const unsigned int PacketCount = 256;
		const unsigned int MaxSequence = 31;		// [0,31]

		ReliableConnection client( ProtocolId, TimeOut, MaxSequence );
		ReliableConnection server( ProtocolId, TimeOut, MaxSequence );

		CHECK( client.Start( ClientPort ) );
		CHECK( server.Start( ServerPort ) );

		client.Connect( Address(127,0,0,1,ServerPort ) );
		server.Listen();

		unsigned int clientAckCount[MaxSequence+1];
		unsigned int serverAckCount[MaxSequence+1];
		for ( unsigned int i = 0; i <= MaxSequence; ++i )
		{
			clientAckCount[i] = 0;
			serverAckCount[i] = 0;
		}

		bool allPacketsAcked = false;

		while ( true )
		{
			if ( !client.IsConnecting() && client.ConnectFailed() )
				break;

			if ( allPacketsAcked )
				break;

			unsigned char packet[256];
			for ( unsigned int i = 0; i < sizeof(packet); ++i )
				packet[i] = (unsigned char) i;

			server.SendPacket( packet, sizeof(packet) );
			client.SendPacket( packet, sizeof(packet) );

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = client.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
				CHECK( bytes_read == sizeof(packet) );
				for ( unsigned int i = 0; i < sizeof(packet); ++i )
					CHECK( packet[i] == (unsigned char) i );
			}

			while ( true )
			{
				unsigned char packet[256];
				int bytes_read = server.ReceivePacket( packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
				CHECK( bytes_read == sizeof(packet) );
				for ( unsigned int i = 0; i < sizeof(packet); ++i )
					CHECK( packet[i] == (unsigned char) i );
			}

			int ack_count = 0;
			unsigned int * acks = NULL;
			client.GetReliabilitySystem().GetAcks( &acks, ack_count );
			CHECK( ack_count == 0 || ack_count != 0 && acks );
			for ( int i = 0; i < ack_count; ++i )
			{
				unsigned int ack = acks[i];
				CHECK( ack <= MaxSequence );
				clientAckCount[ack] += 1;
			}

			server.GetReliabilitySystem().GetAcks( &acks, ack_count );
			CHECK( ack_count == 0 || ack_count != 0 && acks );
			for ( int i = 0; i < ack_count; ++i )
			{
				unsigned int ack = acks[i];
				CHECK( ack <= MaxSequence );
				serverAckCount[ack]++;
			}

			unsigned int totalClientAcks = 0;
			unsigned int totalServerAcks = 0;
			for ( unsigned int i = 0; i <= MaxSequence; ++i )
			{
	 			totalClientAcks += clientAckCount[i];
	 			totalServerAcks += serverAckCount[i];
			}
			allPacketsAcked = totalClientAcks >= PacketCount && totalServerAcks >= PacketCount;

			// note: test above is not very specific, we can do better...

			client.Update( DeltaTime );
			server.Update( DeltaTime );
		}

		CHECK( client.IsConnected() );
		CHECK( server.IsConnected() );
	}

}

// ------------------------------------------------------------------------------------------------------

SUITE( NodeMesh )
{
	TEST( node_connect )
	{
		const int MaxNodes = 2;
		const int MeshPort = 20000;
		const int NodePort = 20001;
		const int ProtocolId = 0x12345678;
		const float DeltaTime = 0.01f;
		const float SendRate = 0.01f;
		const float TimeOut = 1.0f;

		Mesh mesh( ProtocolId, MaxNodes, SendRate, TimeOut );
		CHECK( mesh.Start( MeshPort ) );

		Node node( ProtocolId, SendRate, TimeOut );
		CHECK( node.Start( NodePort ) );

		node.Connect( Address(127,0,0,1,MeshPort) );
		while ( node.IsConnecting() )
		{
			node.Update( DeltaTime );
			mesh.Update( DeltaTime );
		}

		CHECK( !node.ConnectFailed() );

		mesh.Stop();
	}

	TEST( node_connect_fail )
	{
		const int MeshPort = 20000;
		const int NodePort = 20001;
		const int ProtocolId = 0x12345678;
		const float DeltaTime = 0.01f;
		const float SendRate = 0.001f;
		const float TimeOut = 0.1f;

		Node node( ProtocolId, SendRate, TimeOut );
		CHECK( node.Start( NodePort ) );

		node.Connect( Address(127,0,0,1,MeshPort) );
		while ( node.IsConnecting() )
		{
			node.Update( DeltaTime );
		}

		CHECK( node.ConnectFailed() );
	}
	
	TEST( node_connect_busy )
	{
		const int MaxNodes = 1;
		const int MeshPort = 20000;
		const int NodePort = 20001;
		const int ProtocolId = 0x12345678;
		const float DeltaTime = 0.001f;
		const float SendRate = 0.001f;
		const float TimeOut = 0.1f;

		Mesh mesh( ProtocolId, MaxNodes, SendRate, TimeOut );
		CHECK( mesh.Start( MeshPort ) );

		Node node( ProtocolId, SendRate, TimeOut );
		CHECK( node.Start( NodePort ) );

		node.Connect( Address(127,0,0,1,MeshPort) );
		while ( node.IsConnecting() )
		{
			node.Update( DeltaTime );
			mesh.Update( DeltaTime );
		}

		CHECK( !node.ConnectFailed() );

		Node busy( ProtocolId, SendRate, TimeOut );
		CHECK( busy.Start( NodePort + 1 ) );

		busy.Connect( Address(127,0,0,1,MeshPort) );
		while ( busy.IsConnecting() )
		{
			node.Update( DeltaTime );
			busy.Update( DeltaTime );
			mesh.Update( DeltaTime );
		}

		CHECK( busy.ConnectFailed() );
		CHECK( node.IsConnected() );
		CHECK( mesh.IsNodeConnected( 0 ) );

		mesh.Stop();
	}

	TEST( node_connect_multi )
	{
		const int MaxNodes = 4;
		const int MeshPort = 20000;
		const int NodePort = 20001;
		const int ProtocolId = 0x12345678;
		const float DeltaTime = 0.01f;
		const float SendRate = 0.01f;
		const float TimeOut = 1.0f;

		Mesh mesh( ProtocolId, MaxNodes, SendRate, TimeOut );
		CHECK( mesh.Start( MeshPort ) );

		Node * node[MaxNodes];
		for ( int i = 0; i < MaxNodes; ++i )
		{
			node[i] = new Node( ProtocolId, SendRate, TimeOut );
			CHECK( node[i]->Start( NodePort + i ) );
		}

		for ( int i = 0; i < MaxNodes; ++i )
			node[i]->Connect( Address(127,0,0,1,MeshPort) );		

		while ( true )
		{
			bool connecting = false;
			for ( int i = 0; i < MaxNodes; ++i )
			{
				node[i]->Update( DeltaTime );	
				if ( node[i]->IsConnecting() )
					connecting = true;
			}		
			if ( !connecting )
				break;
			mesh.Update( DeltaTime );
		}

		for ( int i = 0; i < MaxNodes; ++i )
		{
			CHECK( !node[i]->IsConnecting() );
			CHECK( !node[i]->ConnectFailed() );
			delete node[i];
		}

		mesh.Stop();
	}

	TEST( node_reconnect )
	{
		const int MaxNodes = 2;
		const int MeshPort = 20000;
		const int NodePort = 20001;
		const int ProtocolId = 0x12345678;
		const float DeltaTime = 0.001f;
		const float SendRate = 0.001f;
		const float TimeOut = 0.1f;

		Mesh mesh( ProtocolId, MaxNodes, SendRate, TimeOut );
		CHECK( mesh.Start( MeshPort ) );

		Node node( ProtocolId, SendRate, TimeOut * 3 );
		CHECK( node.Start( NodePort ) );
		node.Connect( Address(127,0,0,1,MeshPort) );
		while ( node.IsConnecting() )
		{
			node.Update( DeltaTime );
			mesh.Update( DeltaTime );
		}

		node.Stop();
		CHECK( node.Start( NodePort ) );
		node.Connect( Address(127,0,0,1,MeshPort) );
		while ( node.IsConnecting() )
		{
			node.Update( DeltaTime );
			mesh.Update( DeltaTime );
		}

		CHECK( !node.ConnectFailed() );

		mesh.Stop();
	}

	TEST( node_timeout )
	{
		const int MaxNodes = 2;
		const int MeshPort = 20000;
		const int NodePort = 20001;
		const int ProtocolId = 0x12345678;
		const float DeltaTime = 0.001f;
		const float SendRate = 0.001f;
		const float TimeOut = 0.1f;

		Mesh mesh( ProtocolId, MaxNodes, SendRate, TimeOut );
		CHECK( mesh.Start( MeshPort ) );

		Node node( ProtocolId, SendRate, TimeOut );
		CHECK( node.Start( NodePort ) );

		node.Connect( Address(127,0,0,1,MeshPort) );
		while ( node.IsConnecting() || !mesh.IsNodeConnected( 0 ) )
		{
			node.Update( DeltaTime );
			mesh.Update( DeltaTime );
		}

		CHECK( !node.ConnectFailed() );

		int localNodeId = node.GetLocalNodeId();
		int maxNodes = node.GetMaxNodes();

		CHECK( localNodeId == 0 );
		CHECK( maxNodes == MaxNodes );

		CHECK( mesh.IsNodeConnected( localNodeId ) );

		while ( mesh.IsNodeConnected( localNodeId ) )
		{
			mesh.Update( DeltaTime );
		}

		CHECK( !mesh.IsNodeConnected( localNodeId ) );

		while ( node.IsConnected() )
		{
			node.Update( DeltaTime );
		}

		CHECK( !node.IsConnected() );
		CHECK( node.GetLocalNodeId() == -1 );

		mesh.Stop();
	}

	TEST( node_payload )
	{
		const int MaxNodes = 2;
		const int MeshPort = 20000;
		const int ClientPort = 20001;
		const int ServerPort = 20002;
		const int ProtocolId = 0x12345678;
		const float DeltaTime = 0.01f;
		const float SendRate = 0.01f;
		const float TimeOut = 1.0f;

		Mesh mesh( ProtocolId, MaxNodes, SendRate, TimeOut );
		CHECK( mesh.Start( MeshPort ) );

		Node client( ProtocolId, SendRate, TimeOut );
		CHECK( client.Start( ClientPort ) );

		Node server( ProtocolId, SendRate, TimeOut );
		CHECK( server.Start( ServerPort ) );

		mesh.Reserve( 0, Address(127,0,0,1,ServerPort) );

		server.Connect( Address(127,0,0,1,MeshPort) );
		client.Connect( Address(127,0,0,1,MeshPort) );

		bool serverReceivedPacketFromClient = false;
		bool clientReceivedPacketFromServer = false;

		while ( !serverReceivedPacketFromClient || !clientReceivedPacketFromServer )
		{
			if ( client.IsConnected() )
			{
				unsigned char packet[] = "client to server";
				client.SendPacket( 0, packet, sizeof(packet) );
			}

			if ( server.IsConnected() )
			{
				unsigned char packet[] = "server to client";
				server.SendPacket( 1, packet, sizeof(packet) );
			}

			while ( true )
			{
				int nodeId = -1;
				unsigned char packet[256];
				int bytes_read = client.ReceivePacket( nodeId, packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
				if ( nodeId == 0 && strcmp( (const char*) packet, "server to client" ) == 0 )
					clientReceivedPacketFromServer = true;
			}

			while ( true )
			{
				int nodeId = -1;
				unsigned char packet[256];
				int bytes_read = server.ReceivePacket( nodeId, packet, sizeof(packet) );
				if ( bytes_read == 0 )
					break;
				if ( nodeId == 1 && strcmp( (const char*) packet, "client to server" ) == 0 )
					serverReceivedPacketFromClient = true;
			}

			client.Update( DeltaTime );
			server.Update( DeltaTime );

			mesh.Update( DeltaTime );
		}

		CHECK( client.IsConnected() );
		CHECK( server.IsConnected() );

		mesh.Stop();
	}

	TEST( mesh_restart )
	{
		const int MaxNodes = 2;
		const int MeshPort = 20000;
		const int NodePort = 20001;
		const int ProtocolId = 0x12345678;
		const float DeltaTime = 0.001f;
		const float SendRate = 0.001f;
		const float TimeOut = 0.1f;

		Mesh mesh( ProtocolId, MaxNodes, SendRate, TimeOut );
		CHECK( mesh.Start( MeshPort ) );

		Node node( ProtocolId, SendRate, TimeOut );
		CHECK( node.Start( NodePort ) );

		node.Connect( Address(127,0,0,1,MeshPort) );
		while ( node.IsConnecting() )
		{
			node.Update( DeltaTime );
			mesh.Update( DeltaTime );
		}

		CHECK( !node.ConnectFailed() );
		CHECK( node.GetLocalNodeId() == 0 );

		mesh.Stop();

		while ( node.IsConnected() )
		{
			node.Update( DeltaTime );
		}

		CHECK( mesh.Start( MeshPort ) );

		node.Connect( Address(127,0,0,1,MeshPort) );
		while ( node.IsConnecting() )
		{
			node.Update( DeltaTime );
			mesh.Update( DeltaTime );
		}

		CHECK( !node.ConnectFailed() );
		CHECK( node.GetLocalNodeId() == 0 );
	}

	TEST( mesh_nodes )
	{
		const int MaxNodes = 4;
		const int MeshPort = 20000;
		const int NodePort = 20001;
		const int ProtocolId = 0x12345678;
		const float DeltaTime = 0.01f;
		const float SendRate = 0.01f;
		const float TimeOut = 1.0f;

		Mesh mesh( ProtocolId, MaxNodes, SendRate, TimeOut );
		CHECK( mesh.Start( MeshPort ) );

		Node * node[MaxNodes];
		for ( int i = 0; i < MaxNodes; ++i )
		{
			node[i] = new Node( ProtocolId, SendRate, TimeOut );
			CHECK( node[i]->Start( NodePort + i ) );
		}

		for ( int i = 0; i < MaxNodes; ++i )
			node[i]->Connect( Address(127,0,0,1,MeshPort) );		

		while ( true )
		{
			bool connecting = false;
			for ( int i = 0; i < MaxNodes; ++i )
			{
				node[i]->Update( DeltaTime );	
				if ( node[i]->IsConnecting() )
					connecting = true;
			}		
			if ( !connecting )
				break;
			mesh.Update( DeltaTime );
		}

		for ( int i = 0; i < MaxNodes; ++i )
		{
			CHECK( !node[i]->IsConnecting() );
			CHECK( !node[i]->ConnectFailed() );
		}

		// wait for all nodes to get address info for other nodes

		while ( true )
		{
			bool allConnected = true;
			for ( int i = 0; i < MaxNodes; ++i )
			{
				node[i]->Update( DeltaTime );	
				for ( int j = 0; j < MaxNodes; ++j )
				if ( !node[i]->IsNodeConnected( j ) )
					allConnected = false;
			}		
			if ( allConnected )
				break;
			mesh.Update( DeltaTime );
		}

		// verify each node has correct addresses for all nodes

		for ( int i = 0; i < MaxNodes; ++i )
		{
			for ( int j = 0; j < MaxNodes; ++j )
			{
				CHECK( mesh.IsNodeConnected(j) );
				CHECK( node[i]->IsNodeConnected(j) );
				Address a = mesh.GetNodeAddress(j);
				Address b = node[i]->GetNodeAddress(j);
				CHECK( mesh.GetNodeAddress(j) == node[i]->GetNodeAddress(j) );
			}
		}

		// disconnect first node, verify all other node see node disconnect

		node[0]->Stop();

		while ( true )
		{
			bool othersSeeFirstNodeDisconnected = true;
			for ( int i = 1; i < MaxNodes; ++i )
			{
				if ( node[i]->IsNodeConnected(0) )
					othersSeeFirstNodeDisconnected = false;
			}

			bool allOthersConnected = true;
			for ( int i = 1; i < MaxNodes; ++i )
			{
				node[i]->Update( DeltaTime );	
				for ( int j = 1; j < MaxNodes; ++j )
					if ( !node[i]->IsNodeConnected( j ) )
						allOthersConnected = false;
			}		

			if ( othersSeeFirstNodeDisconnected && allOthersConnected )
				break;

			mesh.Update( DeltaTime );
		}

		for ( int i = 1; i < MaxNodes; ++i )
			CHECK( !node[i]->IsNodeConnected(0) );

		for ( int i = 1; i < MaxNodes; ++i )
		{
			for ( int j = 1; j < MaxNodes; ++j )
				CHECK( node[i]->IsNodeConnected( j ) );
		}

		// reconnect node, verify all nodes are connected again

		CHECK( node[0]->Start( NodePort ) );
		node[0]->Connect( Address(127,0,0,1,MeshPort) );		

		while ( true )
		{
			bool connecting = false;
			for ( int i = 0; i < MaxNodes; ++i )
			{
				node[i]->Update( DeltaTime );	
				if ( node[i]->IsConnecting() )
					connecting = true;
			}		
			if ( !connecting )
				break;
			mesh.Update( DeltaTime );
		}

		for ( int i = 0; i < MaxNodes; ++i )
		{
			CHECK( !node[i]->IsConnecting() );
			CHECK( !node[i]->ConnectFailed() );
		}

		while ( true )
		{
			bool allConnected = true;
			for ( int i = 0; i < MaxNodes; ++i )
			{
				node[i]->Update( DeltaTime );	
				for ( int j = 0; j < MaxNodes; ++j )
				if ( !node[i]->IsNodeConnected( j ) )
					allConnected = false;
			}		
			if ( allConnected )
				break;
			mesh.Update( DeltaTime );
		}

		for ( int i = 0; i < MaxNodes; ++i )
		{
			for ( int j = 0; j < MaxNodes; ++j )
			{
				CHECK( mesh.IsNodeConnected(j) );
				CHECK( node[i]->IsNodeConnected(j) );
				Address a = mesh.GetNodeAddress(j);
				Address b = node[i]->GetNodeAddress(j);
				CHECK( mesh.GetNodeAddress(j) == node[i]->GetNodeAddress(j) );
			}
		}

		// cleanup

		for ( int i = 0; i < MaxNodes; ++i )
			delete node[i];

		mesh.Stop();
	}
}

// ------------------------------------------------------------------------------------------------------
