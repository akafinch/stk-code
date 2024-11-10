# main.tf

terraform {
  required_version = ">= 1.0.0"

  required_providers {
    linode = {
      source  = "linode/linode"
      version = "~> 2.29.1"
    }
    kubernetes = {
      source  = "hashicorp/kubernetes"
      version = "~> 2.33.0"
    }
  }
}

# Provider Configuration for Linode
provider "linode" {
  token = var.linode_token
}

# Module for LKE Cluster
module "lke_cluster" {
  source           = "./modules/lke_cluster"
  cluster_label    = var.cluster_label
  cluster_region   = var.cluster_region
  node_pool_type   = var.node_pool_type
  node_pool_count  = var.node_pool_count
}

locals {
  kconf_decoded = base64decode(module.lke_cluster.kube_config_base64)
}

# Provider Configuration for Kubernetes using Module Outputs
provider "kubernetes" {
  config_path = local.kconf_decoded
}

