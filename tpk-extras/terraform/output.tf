output "kubeconfig" {
  value     = nonsensitive(linode_lke_cluster.game_server_cluster.kubeconfig)
  sensitive = true
}

resource "local_file" "kubeconfig" {
  content  =  base64decode(linode_lke_cluster.game_server_cluster.kubeconfig)
  filename = "./kc-${terraform.workspace}"
}
