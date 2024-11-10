# modules/lke_cluster/variables.tf

variable "cluster_label" {
  description = "Label for the Kubernetes cluster"
  type        = string
}

variable "cluster_region" {
  description = "Region for the Kubernetes cluster"
  type        = string
}

variable "node_pool_type" {
  description = "Type of Linode instances for node pool"
  type        = string
}

variable "node_pool_count" {
  description = "Number of nodes in the node pool"
  type        = number
}

