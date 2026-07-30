"""Tests unitarios para bug_reviewer.py y pr_reviewer.py (solo unittest)."""

import os
import sys
import unittest

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
import bug_reviewer  # noqa: E402
import common  # noqa: E402
import pr_reviewer  # noqa: E402


class TestCommentStripping(unittest.TestCase):
    """Regresión del falso positivo real encontrado en accelerations.cuh."""

    def test_line_comment_is_ignored(self):
        code, _in_block = bug_reviewer._strip_comments(
            "cudaFree(x); // cudaMalloc(y) mencionado en un comentario\n", False
        )
        self.assertNotIn("cudaMalloc", code)
        self.assertIsNotNone(bug_reviewer.CUDA_CALL_RE.search(code))
        self.assertEqual(bug_reviewer.CUDA_CALL_RE.search(code).group(1), "cudaFree")

    def test_multiline_block_comment_is_ignored(self):
        lines = [
            "/**\n",
            " * Los lanzadores no llaman cudaDeviceSynchronize().\n",
            " */\n",
            "cudaDeviceSynchronize();\n",
        ]
        in_block = False
        stripped_lines = []
        for line in lines:
            code, in_block = bug_reviewer._strip_comments(line, in_block)
            stripped_lines.append(code)

        # La mención dentro del comentario de documentación no debe generar un match.
        self.assertIsNone(bug_reviewer.CUDA_CALL_RE.search(stripped_lines[1]))
        # La llamada real de código (fuera del comentario) sí debe detectarse.
        self.assertIsNotNone(bug_reviewer.CUDA_CALL_RE.search(stripped_lines[3]))


class TestClassifyChange(unittest.TestCase):
    def test_ci_failed_is_always_human(self):
        mechanical, _reason = pr_reviewer.classify_change(
            ci_success=False, changed_files=["README.md"], pr_body="Closes #7"
        )
        self.assertFalse(mechanical)

    def test_kernel_files_are_always_human(self):
        mechanical, _reason = pr_reviewer.classify_change(
            ci_success=True,
            changed_files=["nbody_2d/kernels/accelerations.cu"],
            pr_body="Closes #7",
        )
        self.assertFalse(mechanical)

    def test_valid_reference_closes_is_mechanical(self):
        mechanical, _reason = pr_reviewer.classify_change(
            ci_success=True, changed_files=["README.md"], pr_body="Closes #7"
        )
        self.assertTrue(mechanical)

    def test_invalid_reference_bare_hash_is_human(self):
        mechanical, _reason = pr_reviewer.classify_change(
            ci_success=True, changed_files=["README.md"], pr_body="ver #7"
        )
        self.assertFalse(mechanical)

    def test_case_insensitive_reference_keywords(self):
        for text in ("closes #7", "FIXES #7", "Resolved #7", "refs #7"):
            with self.subTest(text=text):
                mechanical, _reason = pr_reviewer.classify_change(
                    ci_success=True, changed_files=["README.md"], pr_body=text
                )
                self.assertTrue(mechanical)


class TestBuildCommentGuarantees(unittest.TestCase):
    """El recordatorio de revisión humana no debe depender solo del modelo de IA."""

    def _config_without_provider(self):
        cfg = common.Config()
        cfg.github_token = ""
        cfg.agent_api_url = ""
        cfg.agent_api_key = ""
        return cfg

    def test_non_mechanical_comment_forces_human_notice(self):
        cfg = self._config_without_provider()
        context = {
            "ci_success": False,
            "conclusion": "failure",
            "run_url": "http://example/run/1",
            "changed_files": ["nbody_2d/kernels/accelerations.cu"],
            "mechanical": False,
            "reason": "El CI no terminó en éxito.",
            "diff_excerpt": "",
        }
        comment = pr_reviewer.build_comment("system prompt", cfg, context)
        self.assertIn("Requiere intervención humana", comment)
        self.assertIn("merge de esta PR lo realiza una persona humana", comment)

    def test_mechanical_comment_still_has_merge_reminder(self):
        cfg = self._config_without_provider()
        context = {
            "ci_success": True,
            "conclusion": "success",
            "run_url": "http://example/run/2",
            "changed_files": ["README.md"],
            "mechanical": True,
            "reason": "CI en verde, solo documentación.",
            "diff_excerpt": "",
        }
        comment = pr_reviewer.build_comment("system prompt", cfg, context)
        self.assertIn("merge de esta PR lo realiza una persona humana", comment)


if __name__ == "__main__":
    unittest.main()
