"""Tests unitarios para scripts/agents/common.py (solo unittest + unittest.mock)."""

import io
import os
import socket
import sys
import unittest
import unittest.mock as mock
import urllib.error

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
import common  # noqa: E402


def make_config(dry_run: bool = False) -> common.Config:
    """Config aislada del entorno real (evita depender de variables ambiente)."""
    cfg = common.Config(dry_run=dry_run)
    cfg.github_token = "fake-token-not-a-secret"
    cfg.repo = "owner/repo"
    cfg.agent_api_url = ""
    cfg.agent_api_key = ""
    return cfg


class TestMarkers(unittest.TestCase):
    def test_make_marker_and_find_marker_keys_roundtrip(self):
        marker = common.make_marker("documenter", "sample-key.1")
        text = f"Cuerpo del issue.\n\n{marker}\n"
        keys = common.find_marker_keys(text, "documenter")
        self.assertIn("sample-key.1", keys)

    def test_make_marker_sanitizes_slashes(self):
        marker = common.make_marker("documenter", "nbody_2d/CudaBuffer.h")
        self.assertNotIn("/", marker)
        text = f"body\n{marker}"
        keys = common.find_marker_keys(text, "documenter")
        self.assertEqual(keys, {"nbody_2d-CudaBuffer.h"})

    def test_find_marker_keys_ignores_other_agent(self):
        marker = common.make_marker("bug-reviewer", "some-key")
        keys = common.find_marker_keys(marker, "documenter")
        self.assertEqual(keys, set())


class TestIssueDeduplication(unittest.TestCase):
    def test_marker_exists_true_when_present(self):
        cfg = make_config()
        marker = common.make_marker("documenter", "somekey")
        issues = [{"body": f"texto...\n{marker}", "created_at": "2020-01-01T00:00:00Z"}]
        with mock.patch.object(common, "gh_get", return_value=issues):
            self.assertTrue(
                common.issue_marker_exists(cfg, "agent-documenter", "documenter", "somekey")
            )

    def test_marker_exists_false_when_absent(self):
        cfg = make_config()
        issues = [{"body": "sin marcador aqui", "created_at": "2020-01-01T00:00:00Z"}]
        with mock.patch.object(common, "gh_get", return_value=issues):
            self.assertFalse(
                common.issue_marker_exists(cfg, "agent-documenter", "documenter", "somekey")
            )


class TestWeeklyLimit(unittest.TestCase):
    def test_creates_issue_when_four_recent(self):
        cfg = make_config()
        with mock.patch.object(common, "count_recent_label_issues", return_value=4), \
                mock.patch.object(common, "gh_post", return_value={"html_url": "http://x/1"}) as post:
            result = common.create_issue(
                cfg, "titulo", "cuerpo", ["documentation", "agent-documenter"], "documenter"
            )
        post.assert_called_once()
        self.assertIsNotNone(result)

    def test_skips_issue_when_five_recent(self):
        cfg = make_config()
        with mock.patch.object(common, "count_recent_label_issues", return_value=5), \
                mock.patch.object(common, "gh_post") as post:
            result = common.create_issue(
                cfg, "titulo", "cuerpo", ["documentation", "agent-documenter"], "documenter"
            )
        post.assert_not_called()
        self.assertIsNone(result)

    def test_fail_closed_when_count_check_errors(self):
        cfg = make_config()
        with mock.patch.object(
            common, "count_recent_label_issues", side_effect=common.AgentError("fallo de red")
        ), mock.patch.object(common, "gh_post") as post:
            result = common.create_issue(
                cfg, "titulo", "cuerpo", ["documentation", "agent-documenter"], "documenter"
            )
        post.assert_not_called()
        self.assertIsNone(result)

    def test_counts_by_agent_specific_label(self):
        cfg = make_config()
        with mock.patch.object(common, "count_recent_label_issues", return_value=0) as count, \
                mock.patch.object(common, "gh_post", return_value={"html_url": "http://x/1"}):
            common.create_issue(cfg, "t", "b", ["bug", "agent-bug-reviewer"], "bug-reviewer")
        count.assert_called_once_with(cfg, "agent-bug-reviewer")


class TestDryRunNeverWrites(unittest.TestCase):
    def test_create_issue_dry_run_never_posts(self):
        cfg = make_config(dry_run=True)
        with mock.patch.object(common, "count_recent_label_issues", return_value=0), \
                mock.patch.object(common, "gh_post") as post:
            result = common.create_issue(
                cfg, "titulo", "cuerpo", ["documentation", "agent-documenter"], "documenter"
            )
        post.assert_not_called()
        self.assertIsNone(result)

    def test_create_or_update_pr_comment_dry_run_never_posts_or_patches(self):
        cfg = make_config(dry_run=True)
        with mock.patch.object(common, "list_pr_comments", return_value=[]), \
                mock.patch.object(common, "gh_post") as post, \
                mock.patch.object(common, "gh_patch") as patch:
            result = common.create_or_update_pr_comment(cfg, 1, "cuerpo", "pr-reviewer", "abc123")
        post.assert_not_called()
        patch.assert_not_called()
        self.assertIsNone(result)


class TestHttpErrorHandling(unittest.TestCase):
    def test_http_error_becomes_agent_error(self):
        def raise_http_error(*_args, **_kwargs):
            raise urllib.error.HTTPError(
                "http://x", 404, "Not Found", hdrs=None, fp=io.BytesIO(b'{"message":"nope"}')
            )

        with mock.patch("urllib.request.urlopen", side_effect=raise_http_error):
            with self.assertRaises(common.AgentError):
                common._http_json("GET", "http://x", {})

    def test_http_error_never_leaks_token_in_message(self):
        def raise_http_error(*_args, **_kwargs):
            raise urllib.error.HTTPError(
                "http://x", 401, "Bad credentials", hdrs=None, fp=io.BytesIO(b"unauthorized")
            )

        with mock.patch("urllib.request.urlopen", side_effect=raise_http_error):
            try:
                common._http_json(
                    "GET", "http://x", common.gh_headers("super-secret-token-value")
                )
                self.fail("se esperaba AgentError")
            except common.AgentError as exc:
                self.assertNotIn("super-secret-token-value", str(exc))

    def test_invalid_json_becomes_agent_error(self):
        class FakeResponse:
            status = 200

            def __enter__(self):
                return self

            def __exit__(self, *_exc_info):
                return False

            def read(self):
                return b"esto no es JSON {"

        with mock.patch("urllib.request.urlopen", return_value=FakeResponse()):
            with self.assertRaises(common.AgentError):
                common._http_json("GET", "http://x", {})

    def test_timeout_becomes_agent_error(self):
        with mock.patch("urllib.request.urlopen", side_effect=TimeoutError("timed out")):
            with self.assertRaises(common.AgentError):
                common._http_json("GET", "http://x", {})

    def test_socket_timeout_becomes_agent_error(self):
        with mock.patch("urllib.request.urlopen", side_effect=socket.timeout("timed out")):
            with self.assertRaises(common.AgentError):
                common._http_json("GET", "http://x", {})

    def test_url_error_becomes_agent_error(self):
        with mock.patch(
            "urllib.request.urlopen", side_effect=urllib.error.URLError("no route to host")
        ):
            with self.assertRaises(common.AgentError):
                common._http_json("GET", "http://x", {})


class TestEnsureHumanNotice(unittest.TestCase):
    def test_prepends_phrase_when_missing(self):
        result = common.ensure_human_notice("texto del modelo sin la frase", "motivo de prueba")
        self.assertIn("Requiere intervención humana: motivo de prueba", result)
        self.assertIn("texto del modelo sin la frase", result)

    def test_does_not_duplicate_when_present(self):
        original = "Requiere intervención humana: ya está aquí.\n\nresto del texto"
        result = common.ensure_human_notice(original, "otro motivo")
        self.assertEqual(result, original)


if __name__ == "__main__":
    unittest.main()
