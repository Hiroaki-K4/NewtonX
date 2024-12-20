#include "Octree.hpp"


Octree::Octree(glm::vec3 min_3d_coord, glm::vec3 max_3d_coord) {
    this->root.min_bound = min_3d_coord;
    this->root.max_bound = max_3d_coord;
    this->root.is_leaf = false;
    this->root.is_empty = false;
    subdivide(this->root);
}

Octree::~Octree() {}

bool Octree::is_point_inside(Node &node, const glm::vec3 &position) {
    if (position.x < node.min_bound.x || position.x > node.max_bound.x ||
        position.y < node.min_bound.y || position.y > node.max_bound.y ||
        position.z < node.min_bound.z || position.z > node.max_bound.z) {
        return false;
    }
    return true;
}

void Octree::insert(std::vector<glm::vec3> &position, std::vector<float> &mass) {
    for (int i = 0; i < position.size(); i++) {
        insert_point(this->root, position[i], mass[i]);
    }
}

void Octree::insert_point(Node &node, const glm::vec3 &position, float mass) {
    if (!is_point_inside(node, position)) {
        return;
    }

    if (node.is_leaf) {
        if (node.positions.size() < this->max_points) {
            node.positions.push_back(position);
            node.multi_mass.push_back(mass);
            node.center_of_mass = (
                node.center_of_mass * node.total_mass + position * mass) / (node.total_mass + mass);
            node.total_mass += mass;
        } else {
            subdivide(node);
            for (int i = 0; i < node.positions.size(); i++) {
                for (auto& child : node.children) {
                    if (is_point_inside(child, node.positions[i])) {
                        insert_point(child, node.positions[i], node.multi_mass[i]);
                        break;
                    }
                }
            }
            node.positions.clear();
            node.is_leaf = false;
        }
    } else {
        node.center_of_mass = (
            node.center_of_mass * node.total_mass + position * mass) / (node.total_mass + mass);
        node.total_mass += mass;
        for (auto& child : node.children) {
            if (is_point_inside(child, position)) {
                insert_point(child, position, mass);
                break;
            }
        }
    }
}

void Octree::subdivide(Node &node) {
    glm::vec3 mid((node.min_bound.x + node.max_bound.x) / 2,
                (node.min_bound.y + node.max_bound.y) / 2,
                (node.min_bound.z + node.max_bound.z) / 2);
    // Bottom
    Node new_node;
    create_child_node(
        new_node,
        glm::vec3(node.min_bound.x, node.min_bound.y, mid.z),
        glm::vec3(mid.x, mid.y, node.max_bound.z)
    );
    node.children.push_back(new_node);
    create_child_node(
        new_node,
        glm::vec3(node.min_bound.x, node.min_bound.y, node.min_bound.z),
        glm::vec3(mid.x, mid.y, mid.z)
    );
    node.children.push_back(new_node);
    create_child_node(
        new_node,
        glm::vec3(mid.x, node.min_bound.y, node.min_bound.z),
        glm::vec3(node.max_bound.x, mid.y, mid.z)
    );
    node.children.push_back(new_node);
    create_child_node(
        new_node,
        glm::vec3(mid.x, node.min_bound.y, mid.z),
        glm::vec3(node.max_bound.x, mid.y, node.max_bound.z)
    );
    node.children.push_back(new_node);

    // Top
    create_child_node(
        new_node,
        glm::vec3(node.min_bound.x, mid.y, mid.z),
        glm::vec3(mid.x, node.max_bound.y, node.max_bound.z)
    );
    node.children.push_back(new_node);
    create_child_node(
        new_node,
        glm::vec3(node.min_bound.x, mid.y, node.min_bound.z),
        glm::vec3(mid.x, node.max_bound.y, mid.z)
    );
    node.children.push_back(new_node);
    create_child_node(
        new_node,
        glm::vec3(mid.x, mid.y, node.min_bound.z),
        glm::vec3(node.max_bound.x, node.max_bound.y, mid.z)
    );
    node.children.push_back(new_node);
    create_child_node(
        new_node,
        glm::vec3(mid.x, mid.y, mid.z),
        glm::vec3(node.max_bound.x, node.max_bound.y, node.max_bound.z)
    );
    node.children.push_back(new_node);
}

void Octree::create_child_node(
    Node &new_node, const glm::vec3 &min_bound, const glm::vec3 &max_bound) {
    new_node.min_bound = min_bound;
    new_node.max_bound = max_bound;
}
