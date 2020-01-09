#include <stdio.h>
#include <stdlib.h>
#include "abpoa.h"
#include "abpoa_graph.h"
#include "utils.h"

/* example of dot file for graphviz 
digraph test1 {
    a -> b -> c;
    a -> {x y};
    b [shape=box];
    c [label="hello\nworld",color=blue,fontsize=24,
      fontname="Palatino-Italic",fontcolor=red,style=filled];
    a -> z [label="hi", weight=100];
    x -> z [label="multi-line\nlabel"];
    edge [style=dashed,color=red];
    b -> x;
    {rank=same; b x}
}
graph test2 {
    a -- b -- c [style=dashed];
    a -- {x y};
    x -- c [w=10.0];
    x -- y [w=5.0,len=3];
}
*/

int font_size=24;

// base (index, rank, node_id)
// A (1, 1, 2) A: base 1: index 1: rank 2: node_id
int abpoa_graph_visual(abpoa_graph_t *graph, char *dot_fn) {
    if (graph->is_topological_sorted == 0) err_fatal_simple("Graph is not topologically sorted!.");

    // all settings
    char node_color[5][10] = {"purple3", "red3", "seagreen4", "gold2", "gray"}; // ACGTN
    float dpi_size = 3000, graph_width = 100, graph_height = 6, node_width=1;
    char rankdir[5] = "LR", node_style[10]="filled", node_fixedsize[10]="true", node_shape[10]="circle";
    int show_aligned_mismatch = 1;

    int i, j, id, index, out_id; char base;
    char **node_label = (char**)_err_malloc(graph->node_n * sizeof(char*));
    for (i = 0; i < graph->node_n; ++i) node_label[i] = (char*)_err_malloc(sizeof(char) * 100);
 
    FILE *fp = xopen(dot_fn, "w");
    fprintf(fp, "// ABPOA graph dot file.\n// %d nodes.\n", graph->node_n);
    fprintf(fp, "digraph ABPOA_graph {\n\tgraph [dpi=%f]; size=\"%f,%f\";\n\trankdir=\"%s\";\n\tnode [width=%f, style=%s, fixedsize=%s, shape=%s];\n", dpi_size, graph_width, graph_height, rankdir, node_width, node_style, node_fixedsize, node_shape);

    for (i = 0; i < graph->node_n; ++i) {
        id = abpoa_graph_index_to_node_id(graph, i);
        index = i; // int rank = abpoa_graph_node_id_to_max_rank(graph, id);
        if (id == ABPOA_SRC_NODE_ID) {
            base = 'S';
            //sprintf(node_label[id], "\"%c\n(%d,%d,%d)\"", base, index, rank, id);
            // only show seq
            sprintf(node_label[id], "\"%c\n%d\"", base,index);
            fprintf(fp, "%s [color=%s, fontsize=%d]\n", node_label[id], node_color[4], font_size);
        } else if (id == ABPOA_SINK_NODE_ID) {
            base = 'E';
            //sprintf(node_label[id], "\"%c\n(%d,%d,%d)\"", base, index, rank, id);
            // only show seq
            sprintf(node_label[id], "\"%c\n%d\"", base,index);
            fprintf(fp, "%s [color=%s, fontsize=%d]\n", node_label[id], node_color[4], font_size);
        } else {
            base = "ACGTN"[graph->node[id].base];
            //sprintf(node_label[id], "\"%c\n(%d,%d,%d)\"", base, index, rank, id);
            // only show seq
            sprintf(node_label[id], "\"%c\n%d\"", base,index);
            fprintf(fp, "%s [color=%s, fontsize=%d]\n", node_label[id], node_color[graph->node[id].base], font_size);
        }
    }
    int x_index = -1;
    for (i = 0; i < graph->node_n; ++i) {
        id = abpoa_graph_index_to_node_id(graph, i);
        // out_edge
        for (j = 0; j < graph->node[id].out_edge_n; ++j) {
            out_id = graph->node[id].out_id[j];
            fprintf(fp, "\t%s -> %s [label=\"%d\", penwidth=%d]\n", node_label[id], node_label[out_id], graph->node[id].out_weight[j], graph->node[id].out_weight[j]);
        }
        if (graph->node[id].aligned_node_n > 0) {
            fprintf(fp, "\t{rank=same; %s ", node_label[id]);
            for (j = 0; j < graph->node[id].aligned_node_n; ++j)
                fprintf(fp, "%s ", node_label[graph->node[id].aligned_node_id[j]]);
            fprintf(fp, "};\n");
            if (show_aligned_mismatch) {
                if (i > x_index) {
                    x_index = i;
                    // mismatch dashed line
                    fprintf(fp, "\t{ edge [style=dashed, arrowhead=none]; %s ", node_label[id]);
                    for (j = 0; j < graph->node[id].aligned_node_n; ++j) {
                        fprintf(fp, "-> %s ", node_label[graph->node[id].aligned_node_id[j]]);
                        index = abpoa_graph_node_id_to_index(graph, graph->node[id].aligned_node_id[j]);
                        x_index = index > x_index ? index : x_index;
                    }
                    fprintf(fp, "}\n");
                }
            }
        }
    }
    fprintf(fp, "}\n");

    for (i = 0; i < graph->node_n; ++i) free(node_label[i]); free(node_label);
    err_fclose(fp);

    char cmd[1024];
    sprintf(cmd, "dot %s -Tpdf > %s.pdf", dot_fn, dot_fn);
    if (system(cmd) != 0) err_fatal_simple("Fail to plot ABPOA DAG.");
    return 0;
}
