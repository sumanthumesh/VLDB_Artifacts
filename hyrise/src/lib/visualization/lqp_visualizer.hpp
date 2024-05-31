#pragma once

#include <iomanip>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <boost/algorithm/string.hpp>

#include "expression/abstract_expression.hpp"
#include "logical_query_plan/abstract_lqp_node.hpp"
#include "statistics/cardinality_estimator.hpp"
#include "visualization/abstract_visualizer.hpp"
#include <nlohmann/json.hpp>

namespace hyrise
{

class LQPVisualizer
    : public AbstractVisualizer<std::vector<std::shared_ptr<AbstractLQPNode>>>
{
  public:
    LQPVisualizer();

    LQPVisualizer(GraphvizConfig graphviz_config, VizGraphInfo graph_info = {},
                  VizVertexInfo vertex_info = {}, VizEdgeInfo edge_info = {});

  protected:
    void _build_graph(const std::vector<std::shared_ptr<AbstractLQPNode>>
                          &lqp_roots) override;

    
    std::map<std::string, nlohmann::json::value_type> _build_subtree(const std::shared_ptr<AbstractLQPNode> &node,
                   std::unordered_set<std::shared_ptr<const AbstractLQPNode>>
                       &visualized_nodes,
                   ExpressionUnorderedSet &visualized_sub_queries);

    std::string _build_dataflow(const std::shared_ptr<AbstractLQPNode> &source_node,
                         const std::shared_ptr<AbstractLQPNode> &target_node,
                         const InputSide side);

    CardinalityEstimator _cardinality_estimator;
};

} // namespace hyrise
