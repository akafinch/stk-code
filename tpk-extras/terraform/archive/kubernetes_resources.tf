resource "kubernetes_service" "supertuxkart" {
  metadata {
    name = "supertuxkart-service"
    labels = {
      app = "supertuxkart"
    }
  }

  spec {
    type = "NodePort"

    selector = {
      app = "supertuxkart"
    }

    port {
      name        = "tcp-2757"
      port        = 2757
      target_port = 2757
      protocol    = "TCP"
    }

    port {
      name        = "udp-2757"
      port        = 2757
      target_port = 2757
      protocol    = "UDP"
    }

    port {
      name        = "tcp-2759"
      port        = 2759
      target_port = 2759
      protocol    = "TCP"
    }

    port {
      name        = "udp-2759"
      port        = 2759
      target_port = 2759
      protocol    = "UDP"
    }

    # Optionally: Specify NodePort numbers or let Kubernetes assign them automatically
    # node_port = 30007
  }
}

