# variables.tf

variable "linode_token" {
  description = "Linode API token with appropriate permissions"
  type        = string
  sensitive   = true
}

variable "docker_image" {
  description = "Docker image for the supertuxkart server"
  type        = string
  default     = "akafinch/stk-tpk-server:latest"
}

variable "cluster_label" {
  description = "Label for the Kubernetes cluster"
  type        = string
  default     = "stk-tpk-cluster"
}

variable "cluster_region" {
  description = "Region for the Kubernetes cluster"
  type        = string
  default     = "us-ord" # Chicago for geocentric in NAMER.
}

variable "node_pool_type" {
  description = "Type of Linode instances for node pool"
  type        = string
  default     = "g6-dedicated-8" # 4 CPU, 8GB RAM, dedicated CPU
}

variable "node_pool_count" {
  description = "Number of nodes in the node pool"
  type        = number
  default     = 3
}

variable "kubeconfig_decoded" {
  description = "Decoded Base64 String as provided from LKE provider"
  type        = string
  default     = ""
}

