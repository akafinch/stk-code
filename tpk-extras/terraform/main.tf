resource "linode_lke_cluster" "game_server_cluster" {
  label       = "stk-cluster-${terraform.workspace}"
  region      = var.region
  k8s_version = "1.29"
  tags        = ["stk", "server", "${terraform.workspace}"]
  pool {
    type  = var.node_type
    count = var.node_count

    autoscaler {
      min = var.autoscale_min
      max = var.autoscale_max
    }
  }
}
