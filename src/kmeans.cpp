#include <cstdint>
#include <math.h>
#include <color.h>

double frand() {
    return rand() / (double)RAND_MAX;
}

bool get_most_common_colors(rgb* output, int k, rgb* colors, int nb_colors, int* actual_output_size) {
    const int nb_tests_to_run = 5;
    rgb best_centroids[k];
    double best_deviation = (double)INT32_MAX;

    for (int t = 0; t < nb_tests_to_run; t++) {
        rgb centroids[k];
        rgb* clusters[k];
        int cluster_sizes[k];
        int cluster_capacities[k];
        for (int i = 0; i < k; i++) {
            clusters[i] = NULL;
            rgb_copy(centroids[i], colors[(int)(frand() * nb_colors)]);
        }

        while (true) {
            for (int i = 0; i < k; i++) {
                if (clusters[i] != NULL) {
                    free(clusters[i]);
                    clusters[i] = NULL;
                }
                cluster_sizes[i] = 0;
                cluster_capacities[i] = 0;
            }

            for (int c = 0; c < nb_colors; c++) {
                int target_centroid = find_closest_color_index(colors[c], centroids, k);
                if (target_centroid == -1) {
                    continue;
                }

                if (clusters[target_centroid] == NULL) {
                    clusters[target_centroid] = (rgb*)malloc(sizeof(rgb) * 10);
                    if (clusters[target_centroid] == NULL) {
                        continue;
                    }
                    cluster_capacities[target_centroid] = 10;
                }

                if (cluster_sizes[target_centroid] >= cluster_capacities[target_centroid]) {
                    rgb* tmp_bucket = (rgb*)realloc(clusters[target_centroid], sizeof(rgb) * cluster_capacities[target_centroid] * 2);
                    if (tmp_bucket != NULL) {
                        clusters[target_centroid] = tmp_bucket;
                        cluster_capacities[target_centroid] *= 2;
                    }
                }

                if (cluster_sizes[target_centroid] < cluster_capacities[target_centroid]) {
                    rgb_copy(clusters[target_centroid][cluster_sizes[target_centroid]], colors[c]);
                    cluster_sizes[target_centroid] += 1;
                }
            }

            rgb mean_centroids[k];
            for (int i = 0; i < k; i++) {
                get_average_color(mean_centroids[i], clusters[i], cluster_sizes[i]);
            }

            for (int i = 0; i < k; i++) {
                if (!rgb_equals(centroids[i], mean_centroids[i])) {
                    for (int j = 0; j < k; j++) {
                        rgb_copy(centroids[j], mean_centroids[j]);
                    }
                    continue;
                }
            }
            
            break;
        }

        double sum_deviation = 0;
        for (int i = 0; i < k; i++) {
            for (int c = 0; c < cluster_sizes[i]; c++) {
                sum_deviation += color_distance_squared(clusters[i][c], centroids[i]);
            }
        }

        if (sum_deviation < best_deviation) {
            best_deviation = sum_deviation;
            for (int i = 0; i < k; i++) {
                rgb_copy(best_centroids[i], centroids[i]);
            }
        }

        for (int i = 0; i < k; i++) {
            if (clusters[i] != NULL) {
                free(clusters[i]);
            }
        }
    }


    int skipped = 0;
    for (int i = 0; i < k; i++) {
        bool skip = false;
        for (int j = i - 1; j >= 0; j--) {
            if (rgb_equals(best_centroids[i], best_centroids[j])) {
                skip = true;
                break;
            }
        }

        if (skip) {
            skipped++;
        } else {
            rgb_copy(output[i - skipped], best_centroids[i]);
        }
    }

    *actual_output_size = k - skipped;

    return true;
}

