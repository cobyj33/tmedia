#include "playheadlist.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("PlaylistHead", "[structure]") {
    PlayheadList<int> list;
    
    SECTION("STARTS EMPTY") {
        REQUIRE( list.get_index() == -1 );
        REQUIRE( list.is_empty() == true );
        REQUIRE( list.can_move_index(0));
        REQUIRE( list.can_move_index(1) == false );
        REQUIRE( list.can_move_index(-1) == false ); 
        REQUIRE( list.can_step_forward() == false);
        REQUIRE( list.can_step_backward() == false);
    }

    SECTION("Initial Push Back") {
        list.push_back(5);
        REQUIRE( list.get() == 5 );
        REQUIRE(list.get_index() == 0);
        REQUIRE( list.get_length() == 1 ); 
        list.push_back(3);
        REQUIRE( list.get() == 5 );
        REQUIRE(list.get_index() == 0);
        REQUIRE( list.get_length() == 2 ); 
        list.push_front(2);
        REQUIRE( list.get() == 5 );
        REQUIRE(list.get_index() == 1);
        REQUIRE( list.get_length() == 3 );
    }

    SECTION("Initial Push Front") {
        list.push_front(5);
        REQUIRE( list.get() == 5 );
        REQUIRE(list.get_index() == 0);
        REQUIRE( list.get_length() == 1 ); 
        list.push_back(3);
        REQUIRE( list.get() == 5 );
        REQUIRE(list.get_index() == 0);
        REQUIRE( list.get_length() == 2 ); 
        list.push_front(2); 
        REQUIRE( list.get() == 5 );
        REQUIRE(list.get_index() == 1); // notice how it should shift everything forward when pushed to the front, but the get() value is preserved
        REQUIRE( list.get_length() == 3 );
    }

    SECTION("Data") {
        const int RANGE_10[10] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };

        SECTION("Pushing To Back") {
            for (int i = 0; i < 10; i++) {
                list.push_back(i);
            }

            REQUIRE( list.get_length() == 10 );
            REQUIRE( list.get() == 0 );
            REQUIRE( list.get_index() == 0 );

            SECTION("Setting index") {
                list.set_index(3);

                REQUIRE( list.get_length() == 10 );
                REQUIRE( list.get() == 3 );
                REQUIRE( list.get_index() == 3 );

                SECTION("Setting index backward") {
                    list.set_index(1);
                    REQUIRE( list.get() == 1 );
                    REQUIRE( list.get_index() == 1 );

                }
            }

            SECTION("Stepping Forward") {
                REQUIRE(list.can_step_forward());
                list.step_forward();
                REQUIRE(list.can_step_forward());
                list.step_forward();
                REQUIRE(list.can_step_forward());
                list.step_forward();
                REQUIRE(list.get() == 3);
                REQUIRE(list.get_index() == 3);

                SECTION("Stepping Backward") {
                    REQUIRE(list.can_step_backward());
                    list.step_backward();
                    REQUIRE(list.can_step_backward());
                    list.step_backward();
                    REQUIRE(list.get() == 1);
                    REQUIRE(list.get_index() == 1);
                }
            }
        }


        SECTION("Pushing To Front") {
            for (int i = 0; i < 10; i++) {
                list.push_front(i);
            }

            REQUIRE( list.get_length() == 10 );
            REQUIRE( list.get() == 0 );
            REQUIRE( list.get_index() == 9 );

            SECTION("Try step forward") {
                list.try_step_forward();
                REQUIRE( list.get() == 0 );
                REQUIRE( list.get_index() == 9 );
            }

            SECTION("Try backward forward") {
                list.try_step_backward();
                REQUIRE( list.get() == 1 );
                REQUIRE( list.get_index() == 8 );
            }
        }

    }



}
