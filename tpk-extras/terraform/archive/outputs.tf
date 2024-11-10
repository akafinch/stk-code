# outputs.tf

# output "kube_config" {
#   description = "Kubeconfig to access the Kubernetes cluster"
#   value       = module.lke_cluster.kube_config
# }

# output "external_ips" {
#   description = "External IP addresses assigned to the services"
#   value       = kubernetes_service.supertuxkart-service.load_balancer_ingress[*].ip
# }

output "supertuxkart_service_ports" {
  description = "NodePort numbers for supertuxkart service"
  value = {
    tcp_2757 = kubernetes_service.supertuxkart.spec.0.port[0].node_port
    udp_2757 = kubernetes_service.supertuxkart.spec.0.port[1].node_port
    tcp_2759 = kubernetes_service.supertuxkart.spec.0.port[2].node_port
    udp_2759 = kubernetes_service.supertuxkart.spec.0.port[3].node_port
  }
}

