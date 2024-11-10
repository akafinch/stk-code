# modules/lke_cluster/main.tf

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

resource "linode_lke_cluster" "this" {
  label   = var.cluster_label
  region  = var.cluster_region
  k8s_version = "1.31" # Specify the desired Kubernetes version

  pool {
    type  = var.node_pool_type
    count = var.node_pool_count
  }
}

output "kube_config_base64" {
  description = "Base64-encoded kubeconfig file for the Kubernetes cluster"
  value       = linode_lke_cluster.this.kubeconfig
}
