variable "linode_token" {
  description = "Linode API Token"
  type        = string
}

variable "region" {
  description = "The region to deploy Linode resources"
  type        = string
  default     = "us-ord"
}

variable "node_type" {
  description = "Type of Linode instance for Kubernetes nodes"
  type        = string
  default     = "g6-dedicated-8"
}

variable "node_count" {
  description = "Number of nodes in the Kubernetes cluster"
  type        = number
  default     = 3
}

variable "autoscale_max" {
  description = "Max number of nodes to be spawned during a surge event"
  type        = number
  default     = 10
}

variable "autoscale_min" {
  description = "Min number of nodes to be available during low-load events."
  type        = number
  default     = 3
}
