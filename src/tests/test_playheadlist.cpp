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
        REQUIRE( list.get_length() == 0 );
    }

    SECTION("Initial Push Back") {
        PlayheadList<int> list;
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

        SECTION("Clear") {
            list.clear();
            REQUIRE( list.get_index() == -1 );
            REQUIRE( list.is_empty() == true );
            REQUIRE( list.can_move_index(0));
            REQUIRE( list.can_move_index(1) == false );
            REQUIRE( list.can_move_index(-1) == false ); 
            REQUIRE( list.can_step_forward() == false);
            REQUIRE( list.can_step_backward() == false);
            REQUIRE( list.get_length() == 0 );
        }
    }

    SECTION("Initial Push Front") {
        PlayheadList<int> list;
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

        SECTION("Pop front") {

            SECTION("Index starts at 0") {
                PlayheadList<int> list;
                for (int i = 0; i < 5; i++) {
                    list.push_back(i);
                }

                REQUIRE(list.pop_front() == 0);

                REQUIRE(list.at_edge());
                REQUIRE(list.at_start());
                REQUIRE(list.get_index() == 0);

                REQUIRE(list.pop_front() == 1);

                REQUIRE(list.at_edge());
                REQUIRE(list.at_start());
                REQUIRE(list.get_index() == 0);

                REQUIRE(list.pop_front() == 2);

                REQUIRE(list.at_edge());
                REQUIRE(list.at_start());
                REQUIRE(list.get_index() == 0);

                REQUIRE(list.pop_front() == 3);

                REQUIRE(list.at_edge());
                REQUIRE(list.at_start());
                REQUIRE(list.get_index() == 0);

                REQUIRE(list.pop_front() == 4);

                REQUIRE(list.get_index() == -1);
                REQUIRE(list.is_empty());
                REQUIRE(list.get_length() == 0);

                REQUIRE_THROWS([&](){ list.pop_front(); }());

            }

            SECTION("Index does not start at 0") {
                PlayheadList<int> list;
                for (int i = 0; i < 10; i++) {
                    list.push_back(i);
                }

                list.set_index(4);

                REQUIRE(list.pop_front() == 0);
                REQUIRE(list.get_index() == 3);
                REQUIRE(list.get_length() == 9);

                REQUIRE(list.pop_front() == 1);
                REQUIRE(list.get_index() == 2);
                REQUIRE(list.get_length() == 8);

                REQUIRE(list.pop_front() == 2);
                REQUIRE(list.get_index() == 1);
                REQUIRE(list.get_length() == 7);

                REQUIRE(list.pop_front() == 3);
                REQUIRE(list.get_index() == 0);
                REQUIRE(list.get_length() == 6);

                REQUIRE(list.pop_front() == 4);
                REQUIRE(list.get_index() == 0);
                REQUIRE(list.get_length() == 5);

                REQUIRE(list.pop_front() == 5);
                REQUIRE(list.get_index() == 0);
                REQUIRE(list.get_length() == 4);

                REQUIRE(list.pop_front() == 6);
                REQUIRE(list.get_index() == 0);
                REQUIRE(list.get_length() == 3);
            }

        }

        SECTION("Pushing To Back") {
            PlayheadList<int> list;
            for (int i = 0; i < 10; i++) {
                list.push_back(i);
            }

            REQUIRE( list.get_length() == 10 );
            REQUIRE( list.get() == 0 );
            REQUIRE( list.get_index() == 0 );
        }

        SECTION("Setting index") {
            PlayheadList<int> list;
            for (int i = 0; i < 10; i++) {
                list.push_back(i);
            }

            list.set_index(3);

            REQUIRE( list.get_length() == 10 );
            REQUIRE( list.get() == 3 );
            REQUIRE( list.get_index() == 3 );

            SECTION("Setting index backward") {
                list.set_index(1);
                REQUIRE( list.get() == 1 );
                REQUIRE( list.get_index() == 1 );

            }
        } // SECTION "Setting index"

        SECTION("Stepping Forward") {
            PlayheadList<int> list;
            for (int i = 0; i < 10; i++) {
                list.push_back(i);
            }

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
        } // SECTION "Stepping Forward"


        SECTION("Pushing To Front") {
            PlayheadList<int> list;
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
        } // SECTION "Pushing To Front"

    } // SECTION "Data"
}
